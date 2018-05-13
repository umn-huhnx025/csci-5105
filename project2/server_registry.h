#pragma once

#include <netinet/in.h>
#include <string>
#include <unordered_map>
#include <vector>

typedef struct {
  struct sockaddr_in addr;
  bool isCoord;
} ServerData;

/**
 * This class is used by the coordinator to keep track of the replicas connected
 * to it. It also manages the read and write wuorum sizes for quorum
 * consistency.
 *
 * Our quorum consistency implementation includes a target read/write ratio.
 * Whenever a mew server registers, the number of servers for the read and write
 * quorum's is adjusted auotmatically to be as close to the target ratio as
 * possible while still fulfilling the constraints to prevent read-write and
 * write-write conflicts.
 *
 */
class ServerRegistry {
 public:
  std::unordered_map<std::string, ServerData> servers;

  ServerRegistry(float quoRWRatio);

  int registerNewServer(std::string name, struct sockaddr_in addr);
  int deregister(std::string name);

  std::vector<std::pair<std::string, ServerData>> getWriteQuorum();
  std::vector<std::pair<std::string, ServerData>> getReadQuorum();

 private:
  int numRead;
  int numWrite;
  int numTotal;
  float curRWRatio;
  float goalRWRatio;

  int checkQuora();
  std::vector<std::pair<std::string, ServerData>> getQuorum(int numToKeep);
};
