#include "server_registry.h"

#include <algorithm>
#include <iostream>

ServerRegistry::ServerRegistry(float quoRWRatio)
    : servers(std::unordered_map<std::string, ServerData>()),
      numRead(0),
      numWrite(0),
      numTotal(0),
      curRWRatio(numRead / (float)numWrite),
      goalRWRatio(quoRWRatio) {}

int ServerRegistry::registerNewServer(std::string name,
                                      struct sockaddr_in addr) {
  // Update quorum values
  numTotal++;
  if (numTotal == 1) {
    // First server (coordinator) must be both reader and writer
    numRead++;
    numWrite++;
  } else {
    if (curRWRatio < goalRWRatio) {
      numRead++;
    } else {
      numWrite++;
    }
  }
  curRWRatio = numRead / (float)numWrite;

  // std::cout << "N: " << numTotal << " Nr: " << numRead << " Nw: " << numWrite
  //           << std::endl;

  // Make sure our new quorum values are valid. This check should never fail.
  int check = checkQuora();
  if (check) exit(check);

  bool isCoord = (numTotal == 1);

  servers.emplace(name, (ServerData){addr, isCoord});
  return 0;
}

int ServerRegistry::deregister(std::string name) {
  // Adjust quorum values
  numTotal--;
  if (curRWRatio < goalRWRatio) {
    numWrite--;
  } else {
    numRead--;
    if (numRead < 1) {
      // Make sure we always have at least one server in the read quorum
      numRead = 1;
      numWrite--;
    }
  }
  curRWRatio = numRead / (float)numWrite;

  int check = checkQuora();
  if (check) exit(check);

  // std::cout << "N: " << numTotal << " Nr: " << numRead << " Nw: " << numWrite
  //           << std::endl;

  servers.erase(name);
  return 0;
}

int ServerRegistry::checkQuora() {
  // This should never return non-zero if (de)register sets values properly
  if (numRead + numWrite <= numTotal) {
    std::cerr << "Error: Nr + Nw <= N" << std::endl;
    return 1;
  } else if (numWrite * 2 <= numTotal) {
    std::cerr << "Error: Nw * 2 <= n" << std::endl;
    return 2;
  } else {
    return 0;
  }
}

std::vector<std::pair<std::string, ServerData>> ServerRegistry::getQuorum(
    int numToKeep) {
  // Copy the current registry
  std::vector<std::pair<std::string, ServerData>> s;
  for (auto e : servers) {
    s.emplace_back(e.first, e.second);
  }

  // Randomly remove numTotal - numToKeep servers
  for (int i = 0; i < numTotal - numToKeep; i++) {
    int r = rand() % s.size();
    s.erase(s.begin() + r);
  }

  return s;
}

std::vector<std::pair<std::string, ServerData>>
ServerRegistry::getWriteQuorum() {
  return getQuorum(numWrite);
}

std::vector<std::pair<std::string, ServerData>>
ServerRegistry::getReadQuorum() {
  return getQuorum(numRead);
}
