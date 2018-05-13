#include "tracker.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <thread>

Tracker::Tracker(int port) : port(port) {
  // Initialize endpoint of this server
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  if ((bind(sock, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
    perror("bind");
    exit(1);
  }
}

void Tracker::run() {
  std::cout << "Tracker ready to accept requests" << std::endl;

  while (true) {
    std::thread t;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    char buffer[PACKET_SIZE] = {0};
    int n = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr *)&clientAddr,
                     (socklen_t *)&clientAddrLen);

    std::string request(buffer);

    if (n > 0) {
      t = std::thread([=] { serveClient(request, clientAddr); });
      t.detach();
    } else if (n < 0) {
      perror("recvfrom");
    }
  }
}

void Tracker::serveClient(std::string request, struct sockaddr_in clientAddr) {
  // std::cout << "Received: '" << request << "'" << std::endl;

  // Determine response for request
  std::string response = dispatch(request, clientAddr);

  // Send response in pieces of size PACKET_SIZE
  char *oldres = const_cast<char *>(response.c_str());
  char *res = oldres;

  while (res - oldres < response.length()) {
    char sendbuf[PACKET_SIZE] = {0};
    strncpy(sendbuf, res, PACKET_SIZE);
    int s = sendto(sock, sendbuf, strlen(sendbuf), 0, (sockaddr *)&clientAddr,
                   sizeof(clientAddr));
    res += s;
  }
}

std::string Tracker::dispatch(std::string request,
                              struct sockaddr_in clientAddr) {
  // Create a new socket to keep the main one available for more requests.
  int f;
  if ((f = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }
  struct sockaddr_in fAddr;
  memset(&addr, 0, sizeof(fAddr));
  fAddr.sin_family = AF_INET;
  fAddr.sin_port = htons(0);
  fAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(f, (struct sockaddr *)&fAddr, sizeof(fAddr)) < 0) {
    perror("bind");
    exit(1);
  }

  std::string cmd = request.substr(0, request.find(" "));
  std::string args = request.substr(request.find(" ") + 1);

  std::string ret;

  if (cmd == "find") {
    ret = find(args);
  } else if (cmd == "update") {
    ret = updateList(args);
  }

  close(f);
  return ret;
}

std::string Tracker::find(std::string args) {
  std::string file = args;
  // std::cout << "Finding " << file << std::endl;
  std::string ret;
  for (auto f : files[file]) {
    ret += f + ",";
  }
  ret = ret.empty() ? "Not found" : ret.substr(0, ret.size() - 1);
  return ret;
}

std::string Tracker::updateList(std::string args) {
  std::string peerName = args.substr(0, args.find("\n"));
  std::string list = args.substr(args.find("\n") + 1);

  // std::cout << "Updating files from " << peerName << std::endl;

  // Update the file map
  std::istringstream ss(list);
  std::string file;
  while (std::getline(ss, file)) {
    files[file].insert(peerName);
  }

  return "ok";
}

void Tracker::serverExit() {
  std::cout << "Shutting down server" << std::endl;

  if (close(sock) < 0) {
    perror("Error closing socket");
    exit(1);
  }
}
