#pragma once

#include <netinet/in.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

// The size of our UDP messages
#ifndef PACKET_SIZE
#define PACKET_SIZE 4096
#endif

/**
 * This class represents a tracking server instance. It is responsible for
 * maintaining a list of active peers and keeping track of which files are
 * located at which peers.
 */
class Tracker {
 public:
  Tracker(int port);

  void run();  // Implements main request loop
  void serverExit();

 private:
  int sock, port;  // Endpoint information

  // Map of filenames to peer names
  std::unordered_map<std::string, std::unordered_set<std::string>> files;
  struct sockaddr_in addr;  // The endpoint of this server for clients

  void serveClient(std::string request, struct sockaddr_in clientAddr);
  std::string dispatch(std::string request, struct sockaddr_in clientAddr);

  std::string find(std::string args);
  std::string updateList(std::string args);
};
