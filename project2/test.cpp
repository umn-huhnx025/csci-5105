#include "test.h"

#include <unistd.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>

#include "article.h"
#include "server_registry.h"

static Server** servers;
static Client** clients;
static int nServers, nClients;
static std::ofstream* serverLogs;
static float quoRWRatio;

int run(int nClnt, int nServ, Consistency cons, float ratio) {
  int ret = 0;
  quoRWRatio = ratio;
  nServers = nServ;
  nClients = nClnt;
  if (cons == C_RYW) {
    nServers = std::max(3, nServers);
    nClients = std::max(2, nClients);
  }
  makeServers(cons);
  makeClients();
  switch (cons) {
    case C_SEQ:
    case C_QUO:
      ret = testSeq();
      break;
    case C_RYW:
      ret = testRyw();
      ret |= testSeq();
      break;
  }
  teardown();
  return ret;
}

void teardown() {
  // Coordinator last
  for (int i = nServers - 1; i >= 0; i--) {
    servers[i]->serverExit();
    serverLogs[i].close();
  }
}

void makeServers(Consistency cons) {
  int port = 9100;
  servers = new Server*[nServers];
  serverLogs = new std::ofstream[nServers];

  std::thread t;

  for (int i = 0; i < nServers; i++) {
    std::string logFile = "server_" + std::to_string(i) + ".log";
    serverLogs[i] = std::ofstream(logFile, std::ios::out | std::ios::trunc);

    Server* s;
    if (i == 0) {
      // Coordinator
      s = new Server(port++, cons, serverLogs[i], quoRWRatio);
    } else {
      // Replicas
      s = new Server(port++, servers[0]->name, servers[0]->port, serverLogs[i]);
    }

    servers[i] = s;
    t = std::thread([=] { s->run(); });
    t.detach();
  }
}

void makeClients() {
  clients = new Client*[nClients];
  Server* server;

  for (int i = 0; i < nClients; i++) {
    // Get server name
    // Random
    // int r = rand() % num_servers;

    // Cycle
    int r = i % nServers;

    server = servers[r];

    std::cout << "Making client for server " << server->name << ":"
              << server->port << std::endl;
    Client* c = new Client(server->name, server->port);
    clients[i] = c;
  }
}

int mockWrites(int n, Client* c, int client_id) {
  std::cout << "Client " << client_id << " started posting" << std::endl;
  int numBytes = 0;
  for (int i = 0; i < n; i++) {
    std::string article = "this is post " + std::to_string(i + 1) +
                          " from client " + std::to_string(client_id);
    numBytes += article.length();

    int s = ((float)rand() / RAND_MAX) * client_id * 1000000;
    usleep(s);
    c->post(article);
  }
  return numBytes;
}

int testSeq() {
  // Spawn a number of clients that perform concurrent write operations
  // connected to different servers. After all writes, each client should
  // perform a read and each read should return the same value.

  int ret = 0;

  std::cout << "Testing sequential consistency with " << nServers << " servers"
            << std::endl;

  // for (int i = 0; i < nServers; i++) {
  //   std::cout << servers[i]->name << ":" << servers[i]->port << std::endl;
  // }

  int nWrites = 5;

  std::vector<std::future<int>> nClientWriteBytes;

  auto tStart = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < nClients; i++) {
    nClientWriteBytes.push_back(std::async(std::launch::async, [=] {
      return mockWrites(nWrites, clients[i], i);
    }));
  }

  int nTotalWriteBytes = 0;
  for (int i = 0; i < nClients; i++) {
    // std::cout << "Waiting for client " << i << " to finish" << std::endl;
    nTotalWriteBytes += nClientWriteBytes[i].get();
    std::cout << "Client " << i << " is done posting" << std::endl;
  }

  auto tEnd = std::chrono::high_resolution_clock::now();
  double tWrite =
      std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart)
          .count() /
      1000000.;

  // std::cout << nTotalWriteBytes << " total bytes written" << std::endl;

  std::vector<std::future<std::string>> readFutures;

  tStart = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < nClients; i++) {
    readFutures.push_back(std::async(std::launch::async, [=] {
      std::cout << "Client " << i << " started reading" << std::endl;
      return clients[i]->readAllToString();
    }));
  }

  std::string reads[nClients];
  int nTotalReadBytes = 0;
  for (int i = 0; i < nClients; i++) {
    reads[i] = readFutures[i].get();
    std::cout << "Client " << i << " is done reading" << std::endl;
    nTotalReadBytes += reads[i].length();
  }

  tEnd = std::chrono::high_resolution_clock::now();
  double tRead =
      std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart)
          .count() /
      1000000.;

  int nReadRequests =
      ceil((nClients * nWrites) / Client::READ_PAGE_SIZE + 1) * nClients;

  // std::cout << nTotalReadBytes << " total bytes read" << std::endl;
  // std::cout << "Made " << nReadRequests << " total read requests" <<
  // std::endl;

  for (int i = 1; i < nClients; i++) {
    if (reads[i - 1] != reads[i]) {
      ret = 1;
      std::cout << "FAIL" << std::endl;
      std::cout << "Client " << i - 1 << " read:" << std::endl;
      std::cout << reads[i - 1] << std::endl;
      std::cout << "Client " << i << " read:" << std::endl;
      std::cout << reads[i] << std::endl;
    }
  }

  if (!ret) {
    std::cout << "ok: All clients' reads were identical" << std::endl;
    // std::cout << reads[0] << std::endl;
  } else {
    std::cout << "FAIL" << std::endl;
  }

  std::cout << "Write throughput: " << nClients * nWrites / tWrite
            << " requests/sec, " << nTotalWriteBytes / 1000. / tWrite
            << " KB/sec" << std::endl;
  std::cout << "Read throughput: " << nReadRequests / tRead << " requests/sec, "
            << nTotalReadBytes / 1000. / tRead << " KB/sec" << std::endl;

  return ret;
}

int testRyw() {
  int ret = 0;
  std::cout << "Testing read-your-write consistency\n";

  std::cout << "\n Client will post to server " << servers[1]->name << ":"
            << servers[1]->port << ". \n We will have a propagation delay of"
            << " 2 seconds \n and read from another server before propigation\n"
            << " has been finished to show the write can still be read.\n";
  std::cout << "\nClient posting to " << servers[1]->name
            << "  port: " << servers[1]->port << "\n";

  clients[0]->post("This is a test");
  std::cout << "Client now reading immediately from " << servers[2]->name << ":"
            << servers[2]->port << std::endl;

  std::string output = clients[1]->readAllToString();
  std::string expected = "1: This is a test\n";

  if (!output.compare(expected)) {
    std::cout << "ok: read your write test passed.\n";
    return ret;
  } else {
    std::cout << "FAIL \nexpected: " << expected << "\nGot: " << output;
    return -1;
  }
}
