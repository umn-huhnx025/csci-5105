#include <cstdlib>
#include <cstring>
#include <iostream>

#include "test.h"

int main(int argc, char** argv) {
  srand(time(NULL));

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [seq|quo|ryw]" << std::endl;
    exit(1);
  }

  Consistency cons;
  int nClients = 0;
  int nServers = 0;
  float ratio = -1;

  if (!strncmp(argv[1], "seq", 3)) {
    if (argc < 4) {
      std::cerr << "Usage: " << argv[0] << " seq num_clients num_servers"
                << std::endl;
      exit(1);
    }
    nClients = atoi(argv[2]);
    nServers = atoi(argv[3]);
    cons = C_SEQ;
  } else if (!strncmp(argv[1], "quo", 3)) {
    if (argc < 5) {
      std::cerr << "Usage: " << argv[0]
                << " quo num_clients num_servers rw_ratio" << std::endl;
      exit(1);
    }
    cons = C_QUO;
    nClients = atoi(argv[2]);
    nServers = atoi(argv[3]);
    ratio = atof(argv[4]);
  } else if (!strncmp(argv[1], "ryw", 3)) {
    cons = C_RYW;
    if (argc > 3) {
      nClients = atoi(argv[2]);
      nServers = atoi(argv[3]);
    }
  } else {
    std::cerr << "Unknown test command" << std::endl;
    exit(1);
  }

  return run(nClients, nServers, cons, ratio);
}
