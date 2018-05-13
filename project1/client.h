#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <string>

class Client {
 public:
  int ip;
  int port;
  std::vector<std::string> subs;

  static std::unordered_set<std::string> article_types;

  Client(char* ip_, int port_);
  int send(char* message);
  bool is_subscribed(char* article);
};
