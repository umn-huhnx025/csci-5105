#include "article.h"

#include <iostream>

const int Article::MAXLENGTH = 200;

Article::Article(std::string contents_, int id_)
    : id(id_),
      contents(contents_),
      children(std::vector<Article*>()),
      parent(nullptr) {}

Article* Article::getChild(int id) {
  if (id == this->id) return this;

  for (int i = children.size() - 1; i >= 0; i--) {
    Article* a = children.at(i);
    if (a->id == id) {
      return a;
    } else if (a->id < id) {
      Article* result = a->getChild(id);
      if (result) return result;
    }
  }
  return nullptr;
}

int Article::getDepth() {
  if (!parent) return 0;
  return 1 + parent->getDepth();
}

int Article::getOffset() {
  int offset = 0;

  if (!parent) return offset;

  for (int i = 0; i < parent->children.size(); i++) {
    if (parent->children.at(i) == this) {
      offset += i + 1;
      break;
    }
    offset += parent->children.at(i)->numChildren();
  }

  return offset + parent->getOffset();
}

int Article::numChildren() {
  int num = children.size();
  for (Article* a : children) {
    num += a->numChildren();
  }
  return num;
}

std::string Article::toString() { return std::to_string(id) + ": " + contents; }
