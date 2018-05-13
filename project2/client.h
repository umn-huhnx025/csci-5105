#pragma once

#include <netinet/in.h>
#include <string>

/**
 * This class represents a client instance. It spawns a simple shell that forms
 * requests from user input to send to the server associated with this instance.
 */
class Client {
 public:
  std::string serverName;
  int sock;
  struct sockaddr_in serverAddr;
  int serverPort;

  Client(char* serverName, int serverPort);
  Client(std::string serverName, int serverPort);

  void run();
  void clientExit();

  void post(std::string contents, int parentID = 0);
  void read();
  void readAll();
  std::string readAllToString();
  void choose(int id);

  static const int READ_PAGE_SIZE = 50;

 private:
  std::string getResponse();

  void populate();
};
