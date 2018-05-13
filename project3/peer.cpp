#include "peer.h"

#include <openssl/sha.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <bitset>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
#include <ctime>
#include <time.h>

const int Peer::TRACKER_TIMEOUT = 5;
const int Peer::PEER_TIMEOUT = 5;
const int Peer::HEARTBEAT_INTERVAL = 1;
double Peer::TIME_PASSED = 0;

Peer::Peer(int port, std::string trackerName, int trackerPort, char *share,
           std::string latFile)
    : port(port),
      trackerPort(trackerPort),
      trackerName(trackerName),
      trackerDown(false),
      latencyFile(latFile),
      ready(false),
      listUpdated(false) {
  // Initialize endpoint of this server
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  if ((bind(sock, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
    perror("bind");
    exit(1);
  }

  // Get tracker information
  struct hostent *h = gethostbyname(trackerName.c_str());
  if (h == nullptr) {
    perror("gethostbyname");
    exit(1);
  }

  memcpy(&trackerAddr.sin_addr, h->h_addr_list[0], h->h_length);
  trackerAddr.sin_family = AF_INET;
  trackerAddr.sin_port = htons(trackerPort);

  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  name = std::string(hostname) + ":" + std::to_string(port);

  getcwd(cwd, PATH_MAX);

  // Get shareRoot (~/share, or whatever else)
  if (share) {
    shareRoot = share;
  } else {
    char *chome = getenv("HOME");
    if (!chome) {
      std::cerr << "Please set $HOME" << std::endl;
      exit(1);
    }
    std::string shareStr = std::string(chome) + "/share";
    shareRoot = new char[shareStr.size()];
    strcpy(shareRoot, shareStr.c_str());
  }
}

Peer::~Peer() { delete[] shareRoot; }

void Peer::run(bool interactive) {
  fillLatencyList();

  std::thread cmdThread;
  if (interactive) {
    cmdThread = std::thread([=] { cmd(); });
  }

  std::thread heartbeatThread([=] {
    while (true) {
      updateList();
      if (!listUpdated) {
        {
          std::lock_guard<std::mutex> lk(listLock);
          listUpdated = true;
        }
        listCV.notify_all();
      }
      sleep(HEARTBEAT_INTERVAL);
    }
  });

  // Wait until list is updated once
  std::unique_lock<std::mutex> lk(listLock);
  listCV.wait(lk, [=] { return listUpdated; });
  lk.unlock();

  std::thread trackerRecoverThread([=] { trackerRecover(); });

  // Notify waiters that we are ready
  {
    std::lock_guard<std::mutex> lk(readyLock);
    ready = true;
    //  std::cerr << "Ready to accept requests" << std::endl;
  }
  readyCV.notify_all();

  while (true) {
    std::thread t;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    char buffer[PACKET_SIZE] = {0};
    int n = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr *)&clientAddr,
                     (socklen_t *)&clientAddrLen);

    std::string request(buffer);

    if (n > 0) {
      t = std::thread([=] { serveClient(request, clientAddr); });
      t.detach();
    } else if (n < 0) {
      perror("recvfrom");
    }
  }
}

void Peer::startTests(){
  std::cerr<<"Getting ready. Tests will run in 5 seconds " << std::endl;

  sleep(5);
  std::time_t start = std::time(nullptr);
  for(int i=0;i<=10;i++) {
    get("lorem5000.txt");
  }

  std::cerr << "tests passed in " << std::difftime(std::time(nullptr),start) << "seconds" << std::endl;

}

void Peer::runTests(){
  fillLatencyList();

  std::thread startTestsThread;
  startTestsThread = std::thread([=] { startTests(); });


  std::thread heartbeatThread([=] {
    while (true) {
      updateList();
      if (!listUpdated) {
        {
          std::lock_guard<std::mutex> lk(listLock);
          listUpdated = true;
        }
        listCV.notify_all();
      }
      sleep(HEARTBEAT_INTERVAL);
    }
  });

  // Wait until list is updated once
  std::unique_lock<std::mutex> lk(listLock);
  listCV.wait(lk, [=] { return listUpdated; });
  lk.unlock();

  std::thread trackerRecoverThread([=] { trackerRecover(); });

  // Notify waiters that we are ready
  {
    std::lock_guard<std::mutex> lk(readyLock);
    ready = true;
    //  std::cerr << "Ready to accept requests" << std::endl;
  }
  readyCV.notify_all();

  while (true) {
    std::thread t;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    char buffer[PACKET_SIZE] = {0};
    int n = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr *)&clientAddr,
                     (socklen_t *)&clientAddrLen);

    std::string request(buffer);

    if (n > 0) {
      t = std::thread([=] { serveClient(request, clientAddr); });
      t.detach();
    } else if (n < 0) {
      perror("recvfrom");
    }
  }
}

void Peer::cmd() {
  // Wait until the peer is ready to receive requests
  std::unique_lock<std::mutex> lk(readyLock);
  readyCV.wait(lk, [=] { return ready; });
  lk.unlock();

  while (true) {
    std::string line;
    std::cout << "\rxFS> ";
    getline(std::cin, line);

    if (line.empty()) continue;
    std::string command = line.substr(0, line.find(" "));

    if (command == "get") {
      std::size_t filePos = line.find(" ") + 1;
      if (filePos == 0) {
        std::cout << "Please give the file to find" << std::endl;
        continue;
      }
      std::string file = line.substr(filePos);
      get(file);
    } else if (command == "find") {
      std::size_t filePos = line.find(" ") + 1;
      if (filePos == 0) {
        std::cout << "Please give the file to find" << std::endl;
        continue;
      }
      std::string file = line.substr(filePos);

      std::cout << find(file) << std::endl;
    } else if (command == "update") {
      updateList();
    } else {
      std::cout << "Command unsupported" << std::endl;
    }
  }
}

std::string Peer::findAndRemovePeer(std::string removes, std::string all) {
  // std::cerr<< "removing " + removes << std::endl;
  std::stringstream ss(all);
  std::string peer;
  std::string returnString;
  bool firstTime = true;

  while (std::getline(ss, peer, ',')) {
    if (peer.compare(removes) != 0) {
      if (firstTime) {
        returnString += peer;
        firstTime = false;
      } else
        returnString += "," + peer;
    }
  }

  return returnString;
}

void Peer::get(std::string file, int reqFlag) {
  std::string peers = find(file);

  if (peers == "Not found") {
    std::cout << "Not found" << std::endl;
    return;
  }
  int ret = -1;
  std::string peer;
  std::string bestPeer = "";

  while (ret == -1) {
    // std::cerr << peers << std::endl;
    std::stringstream ss(peers);
    float bestScore = std::numeric_limits<float>::max();
    while (std::getline(ss, peer, ',')) {
      float score = peerDownloadScore(peer);
      if (score == -1) {
        peers = findAndRemovePeer(peer, peers);
        continue;
      }
      if (score < bestScore && score != -1) {
        bestScore = score;
        bestPeer = peer;
      }
    }
    if (bestPeer == "") {
      std::cerr << "No peers with " + file + " online, try again later"
                << std::endl;
      return;
    }
    ret = downloadReq(bestPeer, file, reqFlag);
    //  if(ret == -1) std::cerr << "Unable to download" << std::endl;
    peers = findAndRemovePeer(bestPeer, peers);
    bestPeer = "";
  }

  updateList();
}

float Peer::peerDownloadScore(std::string peer) {
  std::string key = name + ":" + peer;
  std::string s = getLoad(peer);
  if (s == "down") {
    return -1;
  };
  int val = latencyValues[key];
  if (val == 0) val = 5000;  // If not in table, be conservative
  return ((stof(s) * 2) + val);
}

void Peer::serveClient(std::string request, struct sockaddr_in clientAddr) {
  // std::cout << "Received: '" << request << "'" << std::endl;

  // Determine response for request
  std::string response = dispatch(request, clientAddr);

  // Send response in pieces of size PACKET_SIZE
  char *oldres = const_cast<char *>(response.c_str());
  char *res = oldres;
  // std::cout << oldres << std::endl;
  int totalSent = 0;

  while (res - oldres < response.length()) {
    char sendbuf[PACKET_SIZE] = {0};
    strncpy(sendbuf, res, PACKET_SIZE);
    //  std::cout << "send:" <<  sendbuf <<"end of buff" << std::endl;
    int s = sendto(sock, sendbuf, strlen(sendbuf), 0, (sockaddr *)&clientAddr,
                   sizeof(clientAddr));
    // std::cerr << "S is:" << s << std::endl;
    res += s;
  }
}

std::string Peer::dispatch(std::string request, struct sockaddr_in clientAddr) {
  // Create a new socket to keep the main one available for more requests.
  int f;
  struct sockaddr_in fAddr;
  newSocket(&f, &fAddr);

  std::string cmd = request.substr(0, request.find(" "));
  std::string args = request.substr(request.find(" ") + 1);

  std::string ret;
  if (cmd == "download") {
    ret = download(f, args);
  } else if (cmd == "load") {
    ret = std::to_string(load.load());
  }

  close(f);
  return ret;
}

std::string Peer::download(int f, std::string args) {
  //const clock_t begin_time = clock();
  load++;
  std::string file = args;
  // std::cout << "Downloading " << file << std::endl;

  std::stringstream stringstream;
  std::ifstream myfile(std::string(shareRoot) + "/" + args);
  std::string returnString;
  if (myfile.is_open()) {
    stringstream << myfile.rdbuf();
    returnString = stringstream.str();
    myfile.close();
    unsigned char hash[SHA_DIGEST_LENGTH];
    unsigned char *a = reinterpret_cast<unsigned char *>(
        const_cast<char *>(returnString.c_str()));
    SHA1(a, returnString.size(), hash);
    returnString = std::string(hash, hash + SHA_DIGEST_LENGTH) + returnString;
  } else {
    std::cerr << "unable to open: " + args;
  }

  load--;
//  std::cout << float(clock() - begin_time)/CLOCKS_PER_SEC << std::endl;
  return returnString;
}

std::string Peer::bitflip(std::string fileString) {
  std::cout << "Mangling file" << std::endl;
  std::bitset<8> bset(fileString.c_str()[0]);
  bset.flip(2);
  fileString[0] = (char)bset.to_ulong();
  return fileString;
}

int Peer::downloadReq(std::string peer, std::string file, int reqFlag) {
  // We already have the file
  if (peer == name) return -1;

  // Create a new socket to keep the main one available for more requests.
  int f;
  struct sockaddr_in fAddr;
  newSocket(&f, &fAddr, PEER_TIMEOUT);
  struct sockaddr_in peerAddr;

  getPeerAddr(peer, &peerAddr);
  bool firstTimeReading = true;

  std::string request = "download " + file;

  std::string key = name + ":" + peer;
  int sleeptime = latencyValues[key];

  if(sleeptime == 0) sleeptime = 5000;
//  std::cerr <<"Sleeping for" << sleeptime << "Miliseconds"  << std::endl;
  TIME_PASSED += sleeptime;
  usleep( sleeptime * 1000); //network latency

  sendto(f, request.c_str(), request.length(), 0, (sockaddr *)&peerAddr,
         sizeof(peerAddr));





  char buffer[PACKET_SIZE] = {0};
  std::string response;
  int n = recvfrom(f, buffer, sizeof(buffer), 0, NULL, NULL);
  if (n < 0) {
    std::cerr << "Peer down" << std::endl;
    if (errno == EAGAIN) {
      close(f);
      return -1;
    }
    perror("recvfrom");
  }
  std::string totalFileString;
  std::string contents;
  std::string recvHash;

  if (reqFlag == 2) {
    std::cout << "One packet recived, sleeping for 10" << std::endl;
    usleep(10000000);
  }

  while (n > 0) {
    response = std::string(buffer, std::min(sizeof(buffer), strlen(buffer)));

    if (firstTimeReading == true) {
      recvHash = response.substr(0, SHA_DIGEST_LENGTH);
      contents = response.substr(SHA_DIGEST_LENGTH);
      firstTimeReading = false;
    } else {
      contents = response;
    }

    totalFileString += contents;

    // In the future we should have some field in each packet indicating whether
    // or not that packet is the last one of the file. If we don't get to that,
    // we should document that files exactly some multiple of PACKET_SIZE bytes
    // won't be transferred properly. Since that case is super rare, we can
    // probably get away with ignoring it.
    if (n < PACKET_SIZE) break;
    memset(buffer, 0, sizeof(buffer));

    n = recvfrom(f, buffer, sizeof(buffer), 0, NULL, NULL);
    if (n < 0) {
      std::cerr << "Peer down" << std::endl;
      if (errno == EAGAIN) {
        close(f);
        return -1;
      }
      perror("recvfrom");
    }
  }

  if (reqFlag == 1) {
    //  std::cerr<< "Flipping bit " << std::endl;
    totalFileString = bitflip(totalFileString);
  }

  unsigned char hash[SHA_DIGEST_LENGTH];
  unsigned char *a = reinterpret_cast<unsigned char *>(
      const_cast<char *>(totalFileString.c_str()));
  SHA1(a, totalFileString.size(), hash);
  std::string calcHash(hash, hash + SHA_DIGEST_LENGTH);

  if (calcHash == recvHash) {
    std::fstream newfile;
    file = std::string(shareRoot) + "/" + file;
    int namePos = file.find_last_of("/");
    std::string dirname = file.substr(0, namePos);
    std::string filename = file.substr(namePos + 1);
    if (namePos > 0) _mkdir(dirname.c_str());
    newfile.open(file, std::fstream::out);
    newfile << totalFileString;
    newfile.close();
  } else {
    std::cout << "Checksum does not match, trying different server."
              << std::endl;
    return -1;
  }
  return 1;
}

std::string Peer::getLoad(std::string args) {
  // Create a new socket to keep the main one available for more requests.
  int f;
  struct sockaddr_in fAddr;
  newSocket(&f, &fAddr, PEER_TIMEOUT);
  struct sockaddr_in peerAddr;
  getPeerAddr(args, &peerAddr);

  std::string request = "load";
  sendto(f, request.c_str(), request.length(), 0, (sockaddr *)&peerAddr,
         sizeof(peerAddr));

  char buffer[PACKET_SIZE] = {0};
  std::string response;
  int n = recvfrom(f, buffer, sizeof(buffer), 0, NULL, NULL);
  if (n < 0) {
    if (errno == EAGAIN) {
      // std::cerr << "unable to get load" << std::endl;
      close(f);
      return "down";
    }
    perror("recvfrom");
  } else if (n > 0) {
    response = std::string(buffer, std::min(sizeof(buffer), strlen(buffer)));
  }

  return response;
}

std::string Peer::find(std::string file) {
  // Create a new socket to keep the main one available for more requests.
  int f;
  struct sockaddr_in fAddr;
  newSocket(&f, &fAddr, TRACKER_TIMEOUT);

  std::string request = "find " + file;
  sendto(f, request.c_str(), request.length(), 0, (sockaddr *)&trackerAddr,
         sizeof(trackerAddr));

  char buffer[PACKET_SIZE] = {0};
  std::string response;
  int n = recvfrom(f, buffer, sizeof(buffer), 0, NULL, NULL);
  if (n > 0) {
    response = std::string(buffer, std::min(sizeof(buffer), strlen(buffer)));
  } else if (n < 0) {
    if (errno == EAGAIN) {
      close(f);
      waitForTracker();
      return find(file);
    }
    perror("recvfrom");
  }

  close(f);
  return response;
}

void Peer::fillLatencyList() {
  std::string line;
  std::ifstream myfile(latencyFile);
  if (myfile.is_open()) {
    while (getline(myfile, line)) {
      std::string key = line.substr(0, line.find("="));
      std::string value = line.substr(line.find("=") + 1);
       //std::cout << key << std::endl;
      if (value.size() == 0) continue;
      latencyValues[key] = stoi(value);
    }
    myfile.close();
  } else {
    std::cerr << "unable to open " << latencyFile << ": " << strerror(errno)
              << std::endl;
  }
}

int Peer::updateList() {
  // Create a new socket to keep the main one available for more requests.
  int f;
  struct sockaddr_in fAddr;
  newSocket(&f, &fAddr, TRACKER_TIMEOUT);

  std::string list = getList();
  std::string request = "update " + name + "\n" + list;
  std::string response;

  // We should probably account for lists > 4kB at some point
  sendto(f, request.c_str(), request.length(), 0, (sockaddr *)&trackerAddr,
         sizeof(trackerAddr));

  char buffer[PACKET_SIZE] = {0};
  int n = recvfrom(f, buffer, sizeof(buffer), 0, NULL, NULL);
  if (n > 0) {
    response = std::string(buffer, std::min(sizeof(buffer), strlen(buffer)));
  } else if (n < 0) {
    if (errno == EAGAIN) {
      close(f);
      waitForTracker();
      return updateList();
    }
    perror("recvfrom");
  }

  int ret;
  if (response != "ok") {
    ret = 1;
  } else {
    ret = 0;
  }

  close(f);
  return ret;
}

std::string Peer::getList() {
  // Get list of all files in the shared directory
  std::string list;
  list = getListAux(shareRoot);
  return list;
}

std::string Peer::getListAux(char *dir) {
  struct dirent *dp;
  DIR *d;
  std::string list;
  if ((d = opendir(dir)) == NULL) {
    std::cerr << "In dir " << dir << std::endl;
    perror("opendir");
    exit(1);
  }

  while ((dp = readdir(d))) {
    if (!strncmp(dp->d_name, ".", 1) || !strncmp(dp->d_name, "..", 2)) continue;

    std::string relPath = std::string(dp->d_name);
    if (strncmp(dir, ".", 1)) relPath = std::string(dir) + "/" + relPath;

    struct stat st;
    if (stat(relPath.c_str(), &st) < 0) {
      perror("stat");
      exit(1);
    }

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
      list += getListAux(const_cast<char *>(relPath.c_str()));
    } else if ((st.st_mode & S_IFMT) == S_IFREG) {
      std::string r(relPath);
      std::string p = r.substr(strlen(shareRoot) + 1);
      list += p + "\n";
    }
  }

  return list;
}

void Peer::newSocket(int *f, struct sockaddr_in *fAddr, int timeout) {
  // Create a new socket to keep the main one available for more requests.
  if ((*f = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  // Set socket timeout
  if (timeout) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(*f, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      perror("setsockopt timeout");
    }
  }

  memset(&addr, 0, sizeof(*fAddr));
  fAddr->sin_family = AF_INET;
  fAddr->sin_port = htons(0);
  fAddr->sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(*f, (struct sockaddr *)fAddr, sizeof(*fAddr)) < 0) {
    perror("bind");
    exit(1);
  }
}

void Peer::getPeerAddr(std::string peer, struct sockaddr_in *addr) {
  std::string peerHost = peer.substr(0, peer.find(":"));
  //std::cerr << "@@@" << peer.substr(peer.find(":") + 1) << std::endl;
  int peerPort = stoi(peer.substr(peer.find(":") + 1));

  struct hostent *h = gethostbyname(peerHost.c_str());
  if (h == nullptr) {
    perror("gethostbyname");
    exit(1);
  }

  memcpy(&addr->sin_addr, h->h_addr_list[0], h->h_length);
  addr->sin_family = AF_INET;
  addr->sin_port = htons(peerPort);
}

void Peer::trackerRecover() {
  while (true) {
    std::unique_lock<std::mutex> lk(trackerLock);
    trackerCV.wait(lk, [=] { return trackerDown; });
    std::cout << "Tracker is down" << std::endl;

    // Create a new socket to keep the main one available for more requests.
    int f;
    struct sockaddr_in fAddr;
    newSocket(&f, &fAddr, TRACKER_TIMEOUT);

    // Set socket nonblocking
    int flags = fcntl(f, F_GETFL, 0);
    if (flags < 0) {
      perror("fcntl");
      exit(1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(f, F_SETFL, flags) < 0) {
      perror("fcntl");
      exit(1);
    }

    std::string list = getList();
    std::string request = "update " + name + "\n" + list;
    std::string response;

    while (true) {
      // We should probably account for lists > 4kB at some point
      sendto(f, request.c_str(), request.length(), 0, (sockaddr *)&trackerAddr,
             sizeof(trackerAddr));

      char buffer[PACKET_SIZE] = {0};
      int n = recvfrom(f, buffer, sizeof(buffer), 0, NULL, NULL);
      if (n > 0) {
        response =
            std::string(buffer, std::min(sizeof(buffer), strlen(buffer)));
        break;
      } else if (n < 0) {
        if (errno == EAGAIN) {
          sleep(1);
          continue;
        }
        perror("recvfrom");
      }
    }
    close(f);

    trackerDown = false;
    lk.unlock();
    trackerCV.notify_all();
  }
}

void Peer::waitForTracker() {
  {
    std::lock_guard<std::mutex> lk(trackerLock);
    trackerDown = true;
  }
  trackerCV.notify_all();

  {
    std::unique_lock<std::mutex> lk(trackerLock);
    trackerCV.wait(lk, [=] { return !trackerDown; });
    std::cout << "Tracker recovered" << std::endl;
  }
}

void Peer::peerExit() {
  std::cout << "Shutting down server" << std::endl;

  if (close(sock) < 0) {
    perror("Error closing socket");
    exit(1);
  }
}

// Shamelessly lifted from Stack Overflow
// https://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix
void _mkdir(const char *dir) {
  char tmp[256];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", dir);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') tmp[len - 1] = 0;
  for (p = tmp + 1; *p; p++)
    if (*p == '/') {
      *p = 0;
      mkdir(tmp, S_IRWXU);
      *p = '/';
    }
  mkdir(tmp, S_IRWXU);
}
