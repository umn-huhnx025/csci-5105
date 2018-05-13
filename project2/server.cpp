#include "server.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

Server::Server(int port, Consistency cons, int coordPort, std::string coordName,
               bool isCoord, ServerRegistry *sr, std::ostream &os)
    : port(port),
      coordPort(coordPort),
      cons(cons),
      coordName(coordName),
      os(os),
      registry(sr),
      isCoord(isCoord) {
  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  name = std::string(hostname);
  if (isCoord) this->coordName = name;

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

  // Get coordinator information
  struct hostent *h = gethostbyname(coordName.c_str());
  if (h == nullptr) {
    perror("gethostbyname");
    exit(1);
  }

  memcpy(&coordServerAddr.sin_addr, h->h_addr_list[0], h->h_length);
  coordServerAddr.sin_family = AF_INET;
  coordServerAddr.sin_port = htons(coordPort);

  if (!isCoord) {
    // Register with coordinator

    std::string request = "register " + name + ":" + std::to_string(port);

    sockLock.lock();
    sendto(sock, request.c_str(), request.length(), 0,
           (sockaddr *)&coordServerAddr, sizeof(coordServerAddr));

    char buffer[4096] = {0};
    int n = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    if (n < 0) {
      perror("recvfrom");
      exit(1);
    }
    sockLock.unlock();

    std::string response(buffer, std::min(sizeof(buffer), strlen(buffer)));

    // Set consistency policy to the same as the coordinator's
    if (response == "seq") {
      this->cons = C_SEQ;
    } else if (response == "quo") {
      this->cons = C_QUO;
    } else if (response == "ryw") {
      this->cons = C_RYW;
    } else {
      os << "Error registering: " << response << std::endl;
      exit(1);
    }

    os << "Successfully registered with coordinator" << std::endl;

    // Update the bulletin board on this server to the newest version among the
    // group
    std::thread t([=] {
      if (cons == C_QUO) {
        sync();
      }
    });
    t.detach();
  } else {
    // Coordinator should register with itself
    registry->registerNewServer(
        std::string(hostname) + ":" + std::to_string(port), coordServerAddr);
    os << "Registered coordinator" << std::endl;
  }
}

Server::Server(int port, Consistency cons, std::ostream &os, float ratio)
    : Server(port, cons, port, "localhost", true, new ServerRegistry(ratio),
             os) {}

Server::Server(int port, std::string coordName, int coordPort, std::ostream &os)
    : Server(port, C_SEQ, coordPort, coordName, false, nullptr, os) {}

void Server::run() {
  os << "Ready to accept connections" << std::endl;

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

void Server::serveClient(std::string request, struct sockaddr_in clientAddr) {
  // os << "Received: '" << request << "'" << std::endl;

  // Determine response for request
  std::string response = dispatch(request, clientAddr);

  // Send response in pieces of size PACKET_SIZE
  char *oldres = const_cast<char *>(response.c_str());
  char *res = oldres;

  while (res - oldres < response.length()) {
    char sendbuf[PACKET_SIZE] = {0};
    strncpy(sendbuf, res, PACKET_SIZE);
    int s = sendto(sock, sendbuf, strlen(sendbuf), 0, (sockaddr *)&clientAddr,
                   sizeof(clientAddr));
    res += s;
  }
}

std::string Server::dispatch(std::string request,
                             struct sockaddr_in clientAddr) {
  // Create a new socket to keep the main one available for more requests.
  int f;
  if ((f = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }
  struct sockaddr_in fAddr;
  memset(&addr, 0, sizeof(fAddr));
  fAddr.sin_family = AF_INET;
  fAddr.sin_port = htons(0);
  fAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(f, (struct sockaddr *)&fAddr, sizeof(fAddr)) < 0) {
    perror("bind");
    exit(1);
  }

  // Call the real dispatch function based on the current consistency policy
  std::string ret;
  switch (cons) {
    case C_SEQ:
      ret = seqDispatch(request, f, clientAddr);
      break;
    case C_QUO:
      ret = quorumDispatch(request, f, clientAddr);
      break;
    case C_RYW:
      ret = rywDispatch(request, f, clientAddr);
      break;
    default:
      ret = "Error: Invalid consistency policy " + std::to_string(cons);
  }
  close(f);
  return ret;
}

std::string Server::seqDispatch(std::string request, int f,
                                struct sockaddr_in clientAddr) {
  std::string cmd = request.substr(0, request.find(" "));
  std::string args = request.substr(request.find(" ") + 1);

  if (!cmd.compare("post")) {
    if (isCoord) {
      // Forward request to every replica
      request = "s" + request;
      forwardPost(request);
      return "ok";
    } else {
      // Forward request to coordinator
      sendto(f, request.c_str(), request.length(), 0,
             (sockaddr *)&coordServerAddr, sizeof(coordServerAddr));

      char buf[PACKET_SIZE];
      recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
      close(f);
      return std::string(buf);
    }
  } else if (!cmd.compare("spost")) {
    // Perform the post locally
    std::string ret = post(args);
    return ret;
  } else if (!cmd.compare("choose")) {
    // Perform the read locally
    return choose(args);
  } else if (!cmd.compare("read")) {
    // Perform the read locally
    return read(args);
  } else if (cmd == "register") {
    registry->registerNewServer(args, clientAddr);
    return registerNewServer(args);
  } else if (cmd == "deregister") {
    registry->deregister(args);
    os << "Deregistered server at " << args << std::endl;
    return "ok";
  } else {
    return "Error: Invalid command: " + cmd;
  }
}

std::string Server::quorumDispatch(std::string request, int f,
                                   struct sockaddr_in clientAddr) {
  std::string cmd = request.substr(0, request.find(" "));
  std::string args = request.substr(request.find(" ") + 1);

  if (cmd == "post") {
    if (!isCoord) {
      // Forward to coordinator
      m.lock();
      sendto(f, request.c_str(), request.length(), 0,
             (sockaddr *)&coordServerAddr, sizeof(coordServerAddr));
      char buf[PACKET_SIZE] = {0};
      recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
      m.unlock();
      return std::string(buf);
    } else {
      // FOrward request to random write quorum
      auto servers = registry->getWriteQuorum();
      request = "s" + request;
      forwardPost(request, servers);
      return "ok";
    }
  } else if (cmd == "spost") {
    // Perform the post locally
    return post(args);
  } else if (cmd == "read" || cmd == "choose") {
    if (!isCoord) {
      // Forward to coordinator
      m.lock();
      sendto(f, request.c_str(), request.length(), 0,
             (sockaddr *)&coordServerAddr, sizeof(coordServerAddr));
      char buf[PACKET_SIZE] = {0};
      recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
      m.unlock();
      return std::string(buf);
    } else {
      // Get latest version among random read quorum
      auto servers = registry->getReadQuorum();
      std::string result = getLatestRead(request, servers);
      return result;
    }
  } else if (cmd == "sread") {
    return read(args);
  } else if (cmd == "schoose") {
    return choose(args);
  } else if (cmd == "register") {
    registry->registerNewServer(args, clientAddr);
    return registerNewServer(args);
  } else if (cmd == "deregister") {
    registry->deregister(args);
    os << "Deregistered server at " << args << std::endl;
    return "ok";
  } else if (cmd == "version") {
    // Return the local version of the bulletin board
    return std::to_string(articles.nextID);
  } else if (cmd == "sync") {
    return readAllLatest();
  } else if (cmd == "ssync") {
    sync();
    return "ok";
  } else {
    return "Error: Invalid command: " + cmd;
  }
}

std::string Server::rywDispatch(std::string request, int f,
                                struct sockaddr_in clientAddr) {
  std::string cmd = request.substr(0, request.find(" "));
  std::string args = request.substr(request.find(" ") + 1);

  if (cmd == "post") {
    if (isCoord) {
      // Perform the post locally
      std::string ret = post(args);

      // Forward the request to everyone else

      // Make list of servers excluding this one
      std::vector<std::pair<std::string, ServerData>> s;
      for (auto e : registry->servers) {
        if (e.second.isCoord) continue;
        s.emplace_back(e.first, e.second);
      }

      request = "s" + request;
      std::thread t([=] { forwardPost(request, s); });
      t.detach();
      return ret;
    } else {
      // Forward request to coordinator
      sendto(f, request.c_str(), request.length(), 0,
             (sockaddr *)&coordServerAddr, sizeof(coordServerAddr));

      char buf[PACKET_SIZE];
      recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
      close(f);
      return std::string(buf);
    }
  } else if (cmd == "spost") {
    // Perform the post locally
    std::string ret = post(args);
    return ret;
  } else if (cmd == "read" || cmd == "choose") {
    if (isCoord) {
      return read(args);
    } else {
      // Forward request to coordinator
      sendto(f, request.c_str(), request.length(), 0,
             (sockaddr *)&coordServerAddr, sizeof(coordServerAddr));
      char buffer[4096] = {0};
      int n = recvfrom(f, buffer, sizeof(buffer), 0, NULL, NULL);
      return buffer;
    }
  } else if (!cmd.compare("register")) {
    registry->registerNewServer(args, clientAddr);
    return registerNewServer(args);
  } else if (!cmd.compare("deregister")) {
    registry->deregister(args);
    os << "Deregistered server at " << args << std::endl;
    return "ok";
  } else {
    return "Error: Invalid command: " + cmd;
  }
}

void Server::forwardPost(std::string request) {
  std::vector<std::pair<std::string, ServerData>> servers;
  for (auto s : registry->servers) {
    servers.push_back(s);
  }
  forwardPost(request, servers);
}

void Server::forwardPost(
    std::string request,
    std::vector<std::pair<std::string, ServerData>> servers) {
  if (!isCoord) return;

  int f;
  if ((f = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in fAddr;
  memset(&addr, 0, sizeof(fAddr));
  fAddr.sin_family = AF_INET;
  fAddr.sin_port = 0;
  fAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(f, (struct sockaddr *)&fAddr, sizeof(fAddr))) < 0) {
    perror("bind");
    exit(1);
  }

  m.lock();
  if (cons == C_QUO) {
    for (auto &server : servers) {
      std::string syncRequest = "ssync";
      // os << "Forwarding request to " << server.first << ": " << syncRequest
      //    << std::endl;
      sendto(f, syncRequest.c_str(), syncRequest.length(), 0,
             (struct sockaddr *)&server.second.addr,
             sizeof(server.second.addr));

      char buf[PACKET_SIZE] = {0};
      recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
      if (strncmp(buf, "ok", 2)) {
        os << "Forwarded request to " << server.first << " failed: " << buf
           << std::endl;
        m.unlock();
        exit(1);
      }
    }
  }

  for (auto &server : servers) {
    // The long "network dely" will help test RYW consistency because we need to
    // change servers
    if (cons == C_RYW) sleep(2);
    // os << "Forwarding request to " << server.first << ": " << request
    //    << std::endl;
    sendto(f, request.c_str(), request.length(), 0,
           (struct sockaddr *)&server.second.addr, sizeof(server.second.addr));

    char buf[PACKET_SIZE] = {0};
    recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
    if (strncmp(buf, "ok", 2)) {
      os << "Forwarded request to " << server.first << " failed" << std::endl;
      m.unlock();
      exit(1);
    }
  }
  m.unlock();

  close(f);
}

std::string Server::post(std::string args) {
  int parentID = std::stoi(args.substr(0, args.find(" ")));
  // os << "Posting reply to article " << parentID << std::endl;
  std::string contents = args.substr(args.find(" ") + 1);
  // os << "Article contents: " << contents << std::endl;

  articleLock.lock();
  Article *parent = articles.getArticle(parentID);
  if (!parent) {
    articleLock.unlock();
    return "Error: invalid parent article ID: " + std::to_string(parentID);
  }

  Article *a = new Article(contents);
  int e = articles.addArticle(a, parent);
  if (e < 0) {
    std::string message = "Failed to add article to tree";
    std::cerr << message << std::endl;
    articleLock.unlock();
    return "Error: " + message;
  }
  articleLock.unlock();

  os << "Post: " << a->toString() << std::endl;
  return "ok";
}

std::string Server::choose(std::string args) {
  int articleID = std::stoi(args.substr(0, args.find(" ")));

  articleLock.lock();
  Article *a = articles.getArticle(articleID);
  articleLock.unlock();

  if (!a) {
    return "Error: invalid article ID";
  } else {
    os << "Choose: " << a->toString() << std::endl;
    return a->toString();
  }
}

std::string Server::read(std::string args) {
  std::string res;
  int offset = std::stoi(args.substr(0, args.find(" ")));
  std::string s = args.substr(args.find(" ") + 1);
  int count = std::stoi(s.substr(0, s.find(" ")));

  os << "Reading " << count << " articles starting at " << offset + 1
     << std::endl;

  articleLock.lock();
  res += articles.toString(count, offset);
  articleLock.unlock();

  return res.empty() ? "end" : res;
}

std::string Server::registerNewServer(std::string args) {
  os << "Registered new server at " << args << std::endl;
  std::string ret;
  switch (cons) {
    case C_SEQ:
      ret = "seq";
      break;
    case C_QUO:
      ret = "quo";
      break;
    case C_RYW:
      ret = "ryw";
      break;
    default:
      ret = "Error: invalid consistency policy";
      break;
  }
  return ret;
}

std::string Server::getLatestRead(
    std::string request,
    std::vector<std::pair<std::string, ServerData>> servers) {
  int f;
  if ((f = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in fAddr;
  memset(&addr, 0, sizeof(fAddr));
  fAddr.sin_family = AF_INET;
  fAddr.sin_port = 0;
  fAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(f, (struct sockaddr *)&fAddr, sizeof(fAddr))) < 0) {
    perror("bind");
    exit(1);
  }

  std::string vrequest = "version";
  int latest = 0;
  struct sockaddr_in *latestAddr;
  std::string *latestServer;

  m.lock();

  for (auto &server : servers) {
    sendto(f, vrequest.c_str(), vrequest.length(), 0,
           (struct sockaddr *)&server.second.addr, sizeof(server.second.addr));

    char buf[PACKET_SIZE] = {0};
    recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);

    int version = atoi(buf);

    if (version > latest) {
      latest = version;
      latestAddr = &server.second.addr;
      latestServer = &server.first;
    }
  }

  request = "s" + request;
  sendto(f, request.c_str(), request.length(), 0, (struct sockaddr *)latestAddr,
         sizeof(*latestAddr));

  char buf[PACKET_SIZE] = {0};
  recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);

  m.unlock();

  close(f);

  return std::string(buf);
}

void Server::sync() {
  int f;
  if ((f = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in fAddr;
  memset(&addr, 0, sizeof(fAddr));
  fAddr.sin_family = AF_INET;
  fAddr.sin_port = 0;
  fAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(f, (struct sockaddr *)&fAddr, sizeof(fAddr))) < 0) {
    perror("bind");
    exit(1);
  }

  std::string request = "sync";
  sendto(f, request.c_str(), request.length(), 0,
         (struct sockaddr *)&coordServerAddr, sizeof(coordServerAddr));

  char buf[PACKET_SIZE] = {0};
  recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
  close(f);

  std::string result(buf);
  articles = ArticleManager(result);
}

std::string Server::readAllLatest() {
  // Get latest version
  int f;
  if ((f = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in fAddr;
  memset(&addr, 0, sizeof(fAddr));
  fAddr.sin_family = AF_INET;
  fAddr.sin_port = 0;
  fAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(f, (struct sockaddr *)&fAddr, sizeof(fAddr))) < 0) {
    perror("bind");
    exit(1);
  }

  std::string vrequest = "version";
  int latest = 0;
  struct sockaddr_in *latestAddr;
  std::string latestServer;

  for (auto &server : registry->servers) {
    sendto(f, vrequest.c_str(), vrequest.length(), 0,
           (struct sockaddr *)&server.second.addr, sizeof(server.second.addr));

    char buf[PACKET_SIZE] = {0};
    recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);

    int version = atoi(buf);

    if (version > latest) {
      latest = version;
      latestAddr = &server.second.addr;
      latestServer = server.first;
    }
  }

  // Read every article into a string
  int offset = 0;
  int READ_PAGE_SIZE = 50;
  std::string result;
  while (true) {
    std::string request = "sread " + std::to_string(offset) + " " +
                          std::to_string(READ_PAGE_SIZE);

    // std::cout << "Sending: '" << request << "'" << std::endl;
    sendto(f, request.c_str(), request.length(), 0, (sockaddr *)latestAddr,
           sizeof(*latestAddr));

    char buf[PACKET_SIZE] = {0};
    recvfrom(f, buf, PACKET_SIZE, 0, nullptr, nullptr);
    std::string response(buf);

    if (response == "end") {
      break;
    }

    result += response;

    offset += READ_PAGE_SIZE;
  }
  close(f);

  return result.empty() ? "end" : result;
}

void Server::serverExit() {
  sockLock.lock();
  os << "Shutting down server" << std::endl;

  if (!isCoord) {
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      perror("setsockopt timeout");
    }

    std::string request = "deregister " + name + ":" + std::to_string(port);
    sendto(sock, request.c_str(), request.length(), 0,
           (sockaddr *)&coordServerAddr, sizeof(coordServerAddr));

    char buffer[PACKET_SIZE] = {0};
    std::string response;
    int n = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    if (n < 0) {
      perror("recvfrom");
    }
    sockLock.unlock();

    response = std::string(buffer, std::min(sizeof(buffer), strlen(buffer)));
    if (response.compare("ok")) {
      os << "Error deregistering: " << buffer << std::endl;
      exit(1);
    } else {
      os << "Successfully deregistered with coordinator" << std::endl;
    }
  }

  if (close(sock) < 0) {
    perror("Error closing socket");
    exit(1);
  }
}
