#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <iostream>

#include "client.h"

Client* c;

void int_handler(int signal);

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " server_host server_port"
              << std::endl;
    exit(1);
  }

  char* serverName = argv[1];
  int serverPort = atoi(argv[2]);
  c = new Client(serverName, serverPort);

  std::signal(SIGINT, int_handler);
  c->run();
  return 0;
}

void int_handler(int signal) { c->clientExit(); }
