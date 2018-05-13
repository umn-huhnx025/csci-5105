#include "client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <regex>
#include <algorithm>
#include <unordered_set>

std::unordered_set<std::string> Client::article_types = {
    "Sports",     "Lifestyle", "Entertainment", "Business",
    "Technology", "Science",   "Politics",      "Health"};

Client::Client(char *ip_, int port_) : port(port_) {
  inet_pton(AF_INET, ip_, &this->ip);
}

int Client::send(char *message) {
  struct hostent *hp;
  struct sockaddr_in client_addr;

  int sock_fd;
  if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    return 1;
  }

  struct sockaddr_in server_addr;
  memset((char *)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(0);

  if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind failed");
    return 1;
  }

  memset((char *)&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(port);
  client_addr.sin_addr.s_addr = htonl(ip);

  char client_name[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_name,
            INET_ADDRSTRLEN);

  hp = gethostbyname(client_name);
  if (!hp) {
    fprintf(stderr, "could not obtain address of %s\n", client_name);
    return 1;
  }

  memcpy((void *)&client_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

  if (sendto(sock_fd, message, (size_t)strlen(message), 0,
             (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
    perror("sendto failed");
    return 2;
  }

  close(sock_fd);
  return 0;
}

bool Client::is_subscribed(char *article) {
  std::string articleString = std::string(article);
  int count = 0;
  std::string type;
  std::string originator;
  std::string org;
  for (int i = 0; i < articleString.size(); i++) {
    if (articleString[i] != ';') {
      if (count == 0)
        type += articleString[i];
      else if (count == 1)
        originator += articleString[i];
      else if (count == 2)
        org += articleString[i];
      else
        break;
    } else
      count++;
  }
  count = 0;
  for (auto const &s : subs) {
    bool match = true;
    std::string out;
    for (int i = 0; i < s.size(); i++) {
      if (s[i] != ';') {
        out += s[i];
      } else {
        if (count == 0) {
          if (!out.empty() && out.compare(type) != 0) match = false;
          count++;
          out.clear();
        } else if (count == 1) {
          if (!out.empty() && out.compare(originator) != 0) match = false;
          count++;
          out.clear();
        } else if (count == 2) {
          if (!out.empty() && out.compare(org) != 0) match = false;
          out.clear();
        }
      }
    }
    if (match) return true;
  }
  return false;
}
