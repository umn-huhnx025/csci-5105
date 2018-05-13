#pragma once

#include <string>
#include <vector>

class Article {
 public:
  int id;
  std::string contents;
  std::vector<Article*> children;
  Article* parent;

  static const int MAXLENGTH;

  Article(std::string contents_, int id = 0);

  Article* getChild(int id);
  int getOffset();
  int getDepth();
  int numChildren();

  std::string toString();
};
