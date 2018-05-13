#include <unistd.h>
#include <csignal>
#include <cstring>
#include <iostream>

#include "server.h"

Server *s;

void int_handler(int signal);

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0]
              << " port [seq|quo|ryw|<coordinator ip> <coordinator port>]"
              << std::endl;
    exit(1);
  }

  srand(time(NULL));

  int port = atoi(argv[1]);

  Consistency cons = C_SEQ;
  char *coordName = nullptr;
  int coordPort;
  if (!strncmp(argv[2], "seq", 3)) {
    std::cout << "Using sequential consistency" << std::endl;
    cons = C_SEQ;
  } else if (!strncmp(argv[2], "quo", 3)) {
    std::cout << "Using quorum consistency" << std::endl;
    cons = C_QUO;
  } else if (!strncmp(argv[2], "ryw", 3)) {
    std::cout << "Using read-your-write consistency" << std::endl;
    cons = C_RYW;
  } else {
    coordName = argv[2];
    coordPort = atoi(argv[3]);
  }

  if (coordName) {
    s = new Server(port, coordName, coordPort);
  } else if (cons == C_QUO) {
    if (argc < 4) {
      std::cout << "Usage: " << argv[0] << " port quo rw_ratio" << std::endl;
      exit(1);
    }
    float ratio = atof(argv[3]);
    s = new Server(port, cons, std::cout, ratio);
  } else {
    s = new Server(port, cons);
  }

  std::signal(SIGINT, int_handler);
  s->run();

  return 0;
}

void int_handler(int signal) {
  s->serverExit();
  exit(0);
}
