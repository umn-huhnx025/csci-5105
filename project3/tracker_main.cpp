#include <csignal>
#include <iostream>

#include "tracker.h"

Tracker *t;

void int_handler(int signal) {
  t->serverExit();
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " port" << std::endl;
    exit(1);
  }

  int port = atoi(argv[1]);
  t = new Tracker(port);

  std::signal(SIGINT, int_handler);
  t->run();

  return 0;
}
