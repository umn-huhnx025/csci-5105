#include "pubsub_server_test.h"

#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <cassert>

#include "pubsub.h"
#include "pubsub_server.h"

ArticleFormatTest::ArticleFormatTest(std::string article_, int expected_)
    : article(article_), expected(expected_) {}

void test_main() {
  test_subscribe_format();
  test_publish_format();
  test_subscriptions();
  // for (int i = 0; i < MAXCLIENT; i++) {
  //   Client *c = register_new_client();
  //   delete c;
  // }
}

void test_subscribe_format() {
  std::cout << "Testing subscribe format";
  ArticleFormatTest tests[] = {ArticleFormatTest("Sports b c d", 0),
                               ArticleFormatTest(";;;", 0),
                               ArticleFormatTest(";;;content", 0),
                               ArticleFormatTest("Entertainment;;;content", 0),
                               ArticleFormatTest("Business;b;c;content", 0),
                               ArticleFormatTest("Health;;;", 1),
                               ArticleFormatTest("Science;b;c;", 1),
                               ArticleFormatTest(";;c;", 1)};
  int result;
  bool fail = 0;
  for (ArticleFormatTest test : tests) {
    char *a = const_cast<char *>(test.article.c_str());
    result = validate_subscribe(a);
    if (result == test.expected) {
      // std::cout << "ok" << std::endl;
    } else {
      if (!fail) std::cout << std::endl;
      fail = true;
      std::cout << test.article << "... FAIL" << std::endl;
    }
  }
  if (!fail) {
    std::cout << "... ok" << std::endl;
  }
}

void test_publish_format() {
  std::cout << "Testing publish format";
  ArticleFormatTest tests[] = {ArticleFormatTest("Sports b c d", 0),
                               ArticleFormatTest(";;;", 0),
                               ArticleFormatTest(";;;content", 0),
                               ArticleFormatTest("a;b;c;content", 0),
                               ArticleFormatTest("Sports;;;", 0),
                               ArticleFormatTest("Politics;b;c;", 0),
                               ArticleFormatTest("Lifestyle;a;b;c", 1),
                               ArticleFormatTest("Technology;;;c", 1),
                               ArticleFormatTest(";a;b;c", 1),
                               ArticleFormatTest(";;b;c", 1),
                               ArticleFormatTest(";b;;c", 1)};
  int result;
  bool fail;
  for (ArticleFormatTest test : tests) {
    result = validate_publish(const_cast<char *>(test.article.c_str()));
    if (result == test.expected) {
      // std::cout << "ok" << std::endl;
    } else {
      if (!fail) std::cout << std::endl;
      fail = true;
      std::cout << test.article << "... FAIL" << std::endl;
    }
  }
  if (!fail) {
    std::cout << "... ok" << std::endl;
  }
}

Client *register_new_client() {
  int sock_fd;
  if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    exit(1);
  }

  struct sockaddr_in client_addr;

  memset((char *)&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(0);
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  socklen_t len = sizeof(client_addr);
  if (connect(sock_fd, (struct sockaddr *)&client_addr, len) < 0) {
    perror("connect");
    exit(1);
  }

  if (getsockname(sock_fd, (struct sockaddr *)&client_addr, &len) < 0) {
    perror("getsockname failed");
    exit(1);
  }

  char *client_ip = inet_ntoa(client_addr.sin_addr);
  int client_port = ntohs(client_addr.sin_port);
  printf("Made new client %s:%d\n", client_ip, client_port);
  Client *c = new Client(client_ip, client_port);
  c->subs = {"Sports;;;"};
  return c;
}

void test_subscriptions() {
  std::cout << "Testing subscription matching";
  Client c("", 0);

  assert(!c.is_subscribed("Sports;;;content"));
  c.subs = {"Sports;;;"};
  assert(c.is_subscribed("Sports;;;content"));

  c.subs = {"Sports;;UMN;"};
  assert(c.is_subscribed("Sports;;UMN;content"));
  assert(c.is_subscribed("Sports;Somewhere;UMN;content"));
  assert(!c.is_subscribed(";Somewhere;UMN;content"));
  assert(!c.is_subscribed(";;UMN;content"));

  c.subs = {"Sports;;;", "Technology;;;", ";;UMN;"};
  assert(c.is_subscribed("Sports;Somewhere;UMN;content"));
  assert(c.is_subscribed("Technology;Somewhere else;UMN;content"));

  std::cout << "... ok" << std::endl;
}
