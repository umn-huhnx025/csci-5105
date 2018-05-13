#pragma once

#include <string>

#include "client.h"

struct ArticleFormatTest {
  std::string article;
  int expected;
  ArticleFormatTest(std::string article_, int expected_);
};

Client* register_new_client();

void test_main();
void test_subscribe_format();
void test_publish_format();
void test_subscriptions();
