#pragma once

#include <netinet/in.h>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "article_manager.h"
#include "server_registry.h"

// The size of our UDP messages
#define PACKET_SIZE 4096

enum Consistency {
  C_SEQ,
  C_QUO,
  C_RYW,
};

/**
 * This class represents a server instance. It is responsible for manipulating a
 * local article store by communicating with clients and other servers.
 */
class Server {
 public:
  std::string name;
  int sock, port, coordPort;  // Endpoint information
  Consistency cons;
  std::ostream &os;  // Log output stream
  ServerRegistry *registry;
  struct sockaddr_in coordServerAddr;  // Coordinator endpoint
  bool isCoord;
  ArticleManager articles;  // Bulletin board structure

  Server(int port, Consistency cons, int coordPort, std::string coordName,
         bool isCoord, ServerRegistry *sr, std::ostream &os);
  Server(int port, Consistency cons, std::ostream &os = std::cout,
         float ratio = 0.5);
  Server(int port, std::string coordName, int coordPort,
         std::ostream &os = std::cout);

  void run();  // Implements main request loop
  void serverExit();

 private:
  std::string coordName;
  ServerRegistry *sr;
  struct sockaddr_in addr;  // The endpoint of this server for clients

  std::mutex m, articleLock, sockLock;

  // Entrypoint for threads spawned by main request loop
  void serveClient(std::string request, struct sockaddr_in clientAddr);

  // Coordinate subsequent requests made by the server for a client request
  std::string dispatch(std::string request, struct sockaddr_in clientAddr);
  std::string quorumDispatch(std::string request, int f,
                             struct sockaddr_in clientAddr);
  std::string seqDispatch(std::string request, int f,
                          struct sockaddr_in clientAddr);
  std::string rywDispatch(std::string request, int f,
                          struct sockaddr_in clientAddr);

  // Send a post request to other servers
  void forwardPost(std::string request);
  void forwardPost(std::string request,
                   std::vector<std::pair<std::string, ServerData>> servers);

  std::string post(std::string args);
  std::string choose(std::string args);
  std::string read(std::string args);
  std::string registerNewServer(std::string args);

  // Update this server's bulletin board to the newest version among all servers
  void sync();

  // Read the newest version of the bulletin board among all servers
  std::string getLatestRead(
      std::string request,
      std::vector<std::pair<std::string, ServerData>> servers);
  std::string readAllLatest();
};
