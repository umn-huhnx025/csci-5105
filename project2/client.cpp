#include "client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

Client::Client(char *serverName, int serverPort)
    : Client(std::string(serverName), serverPort) {}

Client::Client(std::string serverName, int serverPort)
    : serverName(serverName), serverPort(serverPort) {
  struct hostent *h = gethostbyname(serverName.c_str());
  if (h == nullptr) {
    perror("gethostbyname");
    exit(1);
  }

  memcpy(&serverAddr.sin_addr, h->h_addr_list[0], h->h_length);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(serverPort);

  if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("could not create socket");
    exit(1);
  }

  // std::cout << "Connected to " << this->serverName << std::endl;
}

void Client::run() {
  // populate();
  while (true) {
    std::string line;
    std::cout << "\rboard> ";
    getline(std::cin, line);

    if (line.empty()) continue;
    std::string command = line.substr(0, line.find(" "));

    if (command.compare("post") == 0) {
      std::size_t articlePos = line.find(" ") + 1;
      if (articlePos == 0) {
        std::cout << "Please give the article to post" << std::endl;
        continue;
      }
      std::string contents = line.substr(articlePos);

      post(contents);
    } else if (command.compare("read") == 0) {
      read();
    } else if (command.compare("choose") == 0) {
      std::size_t idPos = line.find(" ") + 1;
      if (idPos == 0) {
        std::cout << "Please give the ID of the chosen article" << std::endl;
        continue;
      }

      int id;
      try {
        id = std::stoi(line.substr(idPos));
      } catch (const std::invalid_argument &ia) {
        std::cerr << "Could not determine article ID" << std::endl;
        continue;
      }

      choose(id);
    } else if (command.compare("reply") == 0) {
      std::size_t idPos = line.find(" ") + 1;
      if (idPos == 0) {
        std::cout << "Please give the ID of the parent article" << std::endl;
        continue;
      }
      int id;
      try {
        id = std::stoi(line.substr(idPos));
      } catch (const std::invalid_argument &ia) {
        std::cerr << "Could not parse parent article ID: " << std::endl;
        continue;
      }
      std::cout << "Parent ID: " << id << std::endl;

      line = line.substr(idPos);
      std::size_t articlePos = line.find(" ") + 1;
      if (articlePos == 0) {
        std::cout << "Please give the article to post" << std::endl;
        continue;
      }
      std::string contents = line.substr(articlePos);

      post(contents, id);
    } else if (command.compare("q") == 0) {
      break;
    } else {
      std::cout << "Command unsupported" << std::endl;
    }
  }
  clientExit();
}

void Client::post(std::string contents, int parentID) {
  // std::cout << "Posting article";
  // if (parentID) std::cout << " in reply to " << parentID;
  // std::cout << std::endl;

  std::string request = "post " + std::to_string(parentID) + " " + contents;
  sendto(sock, request.c_str(), request.length(), 0, (sockaddr *)&serverAddr,
         sizeof(serverAddr));

  std::string response = getResponse();
  if (!response.empty()) std::cerr << response << std::endl;
}

void Client::read() {
  // std::cout << "Reading articles" << std::endl;

  int offset = 0;
  int READ_PAGE_SIZE = 50;

  while (true) {
    std::string request =
        "read " + std::to_string(offset) + " " + std::to_string(READ_PAGE_SIZE);
    sendto(sock, request.c_str(), request.length(), 0, (sockaddr *)&serverAddr,
           sizeof(serverAddr));

    std::string response = getResponse();

    if (response == "end") {
      std::cout << "No more articles" << std::endl;
      break;
    }

    std::cout << response;

    offset += READ_PAGE_SIZE;

    std::cout << "Load " << READ_PAGE_SIZE << " more articles? (y/n) ";
    std::string ans;
    std::cin >> ans;
    if (ans != "y" && ans != "Y") break;
  }
}

void Client::choose(int id) {
  // std::cout << "Choosing article " << id << std::endl;

  std::string request = "choose " + std::to_string(id);
  sendto(sock, request.c_str(), request.length(), 0, (sockaddr *)&serverAddr,
         sizeof(serverAddr));

  std::string response = getResponse();

  std::cout << response << std::endl;
}

std::string Client::getResponse() {
  // std::cout << "Waiting for response" << std::endl;
  char buffer[4096] = {0};
  std::string response;
  int n = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
  if (n > 0) {
    response = std::string(buffer, std::min(sizeof(buffer), strlen(buffer)));
  } else if (n < 0) {
    perror("recvfrom");
  }

  if (!strncmp(buffer, "ok", 2))
    return "";
  else
    return response;
}

void Client::clientExit() {
  std::cout << "Closing client" << std::endl;
  if (close(sock) < 0) {
    perror("Error closing socket");
    exit(1);
  }
  exit(0);
}

void Client::readAll() { std::cout << readAllToString(); }

std::string Client::readAllToString() {
  int offset = 0;
  int READ_PAGE_SIZE = 50;
  std::string result;

  while (true) {
    std::string request =
        "read " + std::to_string(offset) + " " + std::to_string(READ_PAGE_SIZE);

    sendto(sock, request.c_str(), request.length(), 0, (sockaddr *)&serverAddr,
           sizeof(serverAddr));

    std::string response = getResponse();

    if (response == "end") {
      break;
    }

    result += response;

    offset += READ_PAGE_SIZE;
  }
  return result;
}

void Client::populate() {
  srand(time(NULL));
  int *result;
  int numArticles = 10;
  for (int i = 0; i < numArticles; i++) {
    if (rand() % 5 && i) {
      int id = rand() % i + 1;
      post("this is reply " + std::to_string(i + 1) + " to " +
               std::to_string(id),
           id);
    } else {
      post("this is post " + std::to_string(i + 1));
    }
  }
  readAll();
}
