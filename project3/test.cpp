#include <unistd.h>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "peer.h"

static const int TRACKER_PORT = 9100;

// Relative to $HOME
static const std::string PEER1_SHARE = "share1";
static const std::string PEER2_SHARE = "share2";

std::string readFile(std::string file) {
  std::ifstream f(file);
  std::stringstream ss;

  if (!f.is_open()) {
    std::cerr << "Error opening " << file << ": " << strerror(errno)
              << std::endl;
    return "";
  }

  ss << f.rdbuf();
  std::string result = ss.str();
  f.close();
  return result;
}

void kill_peer(int ppid) {
  sleep(5);
  std::cerr << "Killing peer currect sending data" << std::endl;
  if (kill(ppid, SIGINT) < 0) {
    perror("kill");
    exit(1);
  }
}

void getAndDiff(Peer *pr, std::string file, int reqFlag = 0) {
  pr->get(file, reqFlag);

  std::string fn1 = "test_files/" + file;
  std::string fn2 = std::string(pr->shareRoot) + "/" + file;
  std::string f1 = readFile(fn1);
  std::string f2 = readFile(fn2);

  if (f1 == f2) {
    std::cout << "ok" << std::endl;
  } else {
    std::cout << "FAIL" << std::endl;
  }
}

int main(int argc, char *argv[]) {
  std::cout << "Running all tests" << std::endl;

  // Setup
  std::cout << "Creating servers" << std::endl;

  // Get shared directory
  char *chome = getenv("HOME");
  if (!chome) {
    std::cerr << "Please set $HOME" << std::endl;
    exit(1);
  }
  std::string shareStr = std::string(chome) + "/" + PEER2_SHARE;
  char *shareRoot = new char[shareStr.size()];
  strcpy(shareRoot, shareStr.c_str());

  // Receiving peer
  Peer *p = new Peer(TRACKER_PORT + 2, "localhost", TRACKER_PORT, shareRoot,
                     "test_latency.txt");

  // Launch servers
  int tpid = fork();
  if (tpid == 0) {
    execl("./tracker", "tracker", std::to_string(TRACKER_PORT).c_str(), NULL);
    perror("exec");
    exit(1);
  }

  std::cout << "Waiting for tracker to start" << std::endl;
  sleep(1);

  // Sending peer
  int ppid = fork();
  if (ppid == 0) {
    shareStr = std::string(chome) + "/" + PEER1_SHARE;
    execl("./peer", "peer", std::to_string(TRACKER_PORT + 1).c_str(),
          "localhost", std::to_string(TRACKER_PORT).c_str(), shareStr.c_str(),
          NULL);
    perror("exec");
    exit(1);
  }

  std::cout << "Waiting for sending peer to start" << std::endl;
  sleep(1);

  std::thread pThr;
  pThr = std::thread([=] { p->run(false); });
  pThr.detach();

  // Wait until the peer is ready to receive requests
  std::unique_lock<std::mutex> lk(p->readyLock);
  p->readyCV.wait(lk, [=] { return p->ready; });
  lk.unlock();

  // Test file transfers
  std::cout << "Testing file transfers" << std::endl;

  std::cout << "This file should not be found" << std::endl;
  p->get("does_not_exist.txt");

  std::cout << "Getting a regular file" << std::endl;
  getAndDiff(p, "f1.txt");

  std::cout << "Getting a large file" << std::endl;
  getAndDiff(p, "lorem100000.txt");

  std::cout << "Getting a file inside a directory" << std::endl;
  getAndDiff(p, "dir1/dir2/f4.txt");

  std::cout << "Getting a file with a bad checksum" << std::endl;
  getAndDiff(p, "lorem5000.txt", 1);

  // Test tracker recovery
  std::cout << "Killing tracker" << std::endl;
  if (kill(tpid, SIGINT) < 0) {
    perror("kill");
    exit(1);
  }

  std::cout << "Sleeping for 5 seconds" << std::endl;
  sleep(5);

  tpid = fork();
  if (tpid == 0) {
    std::cout << "Restarting tracker" << std::endl;
    execl("./tracker", "tracker", std::to_string(TRACKER_PORT).c_str(), NULL);
    perror("exec");
    exit(1);
  }

  std::cout << "Sleeping for 5 seconds" << std::endl;
  sleep(5);

  // Test peer recovery
  std::cout << "Testing peer recovery" << std::endl;

  std::cout << "Killing sending peer" << std::endl;
  if (kill(ppid, SIGINT) < 0) {
    perror("kill");
    exit(1);
  }
  sleep(1);

  std::cout << "Downloading unique file" << std::endl;
  getAndDiff(p, "unique.txt");

  ppid = fork();
  if (ppid == 0) {
    std::cout << "Restarting sending peer" << std::endl;
    shareStr = std::string(chome) + "/" + PEER1_SHARE;
    execl("./peer", "peer", std::to_string(TRACKER_PORT + 1).c_str(),
          "localhost", std::to_string(TRACKER_PORT).c_str(), shareStr.c_str(),
          NULL);
    perror("exec");
    exit(1);
  }

  sleep(1);

  std::cout << "Trying request again" << std::endl;
  getAndDiff(p, "unique.txt");

  std::thread killer;
  std::cout << "Creating a killer thread that will kill the peer in 5 seconds"
            << std::endl;
  killer = std::thread([=] { kill_peer(ppid); });
  killer.detach();

  std::cout
      << "Testing if peer dies in middle of sending file, this should fail"
      << std::endl;
  getAndDiff(p, "unique2.txt", 2);

  // Tear down
  std::cout << "Tearing down servers" << std::endl;
  p->peerExit();
  delete p;

  if (kill(tpid, SIGINT) < 0) {
    perror("kill");
    exit(1);
  }

  return 0;
}
