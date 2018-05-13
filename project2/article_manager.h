#pragma once

#include "article.h"

/**
 * This class represents a bulletin board. It forms a tree of articles, where
 * child articles are replies to their parents. Every article is a child of the
 * root article of the tree.
 */
class ArticleManager {
 public:
  Article* root;
  int nextID;
  ArticleManager();
  ArticleManager(std::string allArticles);

  int addArticle(Article* a, Article* parent = nullptr, int id = -1);

  Article* getArticle(int id);

  std::string toString(int count, int offset);

 private:
  static const int ROOT_ID;
};
