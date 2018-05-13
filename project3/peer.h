#pragma once

#include <limits.h>
#include <netinet/in.h>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

// The size of our UDP messages
#ifndef PACKET_SIZE
#define PACKET_SIZE 126
#endif

/**
 * This class represents a peer instance. It is responsible for communicating
 * with the tracker server and other peers to coordinate downloading files.
 */
class Peer {
 public:
  Peer(int port, std::string trackerName, int trackerPort, char *share,
       std::string latFile = "latency.txt");
  virtual ~Peer();

  void run(bool interactive = true);  // Implements main request loop

  // Wrapper around other functions to select a peer and download from it.
  void get(std::string file, int reqFlag = 0);

  void peerExit();  // Cleanly exit the peer.

  std::string name;                // Host:port name for this peer
  int sock, port, trackerPort;     // Endpoint information
  struct sockaddr_in trackerAddr;  // tracker endpoint
  std::string trackerName;         // Hostname of the tracker
  struct sockaddr_in addr;         // The endpoint of this server for clients
  std::atomic_int load;            // Number of concurrent downloads
  char *shareRoot;
  std::unordered_map<std::string, int> latencyValues;
  std::string latencyFile;
  char cwd[PATH_MAX];

  bool trackerDown;
  std::mutex trackerLock;
  std::condition_variable trackerCV;

  bool listUpdated;
  std::mutex listLock;
  std::condition_variable listCV;

  bool ready;
  std::mutex readyLock;
  std::condition_variable readyCV;

  static const int TRACKER_TIMEOUT;
  static const int PEER_TIMEOUT;
  static const int HEARTBEAT_INTERVAL;
  static double TIME_PASSED;

  void cmd();  // CLI loop

  // Find which peers a given file is on.
  std::string find(std::string file);

  // Choose the best peer to download a file from given a list.
  std::string selectPeer(std::string peers);

  // Compute metric representing a peer's ability to download a file using
  // latency and load. Higher is better.
  float peerDownloadScore(std::string peer);

  // Send a download request to another peer.
  int downloadReq(std::string peer, std::string file, int reqFlag = 0);

  // Send the tracker an updated list of files in our shared directory.
  int updateList();

  // fills list of letency pairs.
  void fillLatencyList();

  // Get the list of files in the shared directory for this peer.
  std::string getList();

  std::string getListAux(char *dir);

  // Entrypoint for threads spawned by main request loop.
  void serveClient(std::string request, struct sockaddr_in clientAddr);

  // Coordinate subsequent requests made by the server for a client request
  std::string dispatch(std::string request, struct sockaddr_in clientAddr);

  // Service a download request from another
  std::string download(int f, std::string args);

  // Return the current load on this server.
  std::string getLoad(std::string args);

  // Create a new socket.
  void newSocket(int *f, struct sockaddr_in *fAddr, int timeout = 0);

  // Get a new address struct for a peer.
  void getPeerAddr(std::string peer, struct sockaddr_in *addr);

  // Use this to flip one bit of the string
  std::string bitflip(std::string);

  std::string findAndRemovePeer(std::string, std::string);

  void startTests();

  void runTests();

  void trackerRecover();
  void waitForTracker();
};

// Recursive mkdir
void _mkdir(const char *dir);
