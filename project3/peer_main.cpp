#include <csignal>
#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>

#include "peer.h"

Peer *p;

void int_handler(int signal) {
  p->peerExit();
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cout << "Usage: " << argv[0]
              << " port tracker_name tracker_port [shared_dir]" << std::endl;
    exit(1);
  }

  int port = atoi(argv[1]);
  std::string trackerName = argv[2];
  int trackerPort = atoi(argv[3]);
  char *shareRoot = nullptr;
  if (argc > 4 ) shareRoot = argv[4];

  p = new Peer(port, trackerName, trackerPort, shareRoot);

  std::signal(SIGINT, int_handler);

  if(argc > 5){
    if(!strcmp(argv[5],"-t")){
      p->run(false);
    } else if(!strcmp(argv[5],"-r")){
      p->runTests();
    }else {
      std::cerr<<"5th argument is the test flag (-t | -r) " << std::endl;
    }
  } else {
    p->run();
  }
  return 0;
}
