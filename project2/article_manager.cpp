#include "article_manager.h"

#include <iostream>
#include <sstream>
#include <stack>

const int ArticleManager::ROOT_ID = 0;

ArticleManager::ArticleManager()
    : root(new Article("[root]", ROOT_ID)), nextID(ROOT_ID + 1) {}

ArticleManager::ArticleManager(std::string allArticles) : ArticleManager() {
  if (allArticles == "end") return;
  std::istringstream s(allArticles);
  std::string line;
  int lastIndent = -1;
  Article* lastArticle = root;
  std::stack<Article*> parents;
  while (std::getline(s, line)) {
    int i;
    for (i = 0; i < line.length(); i++) {
      if (line[i] != ' ') break;
    }
    line = line.substr(i);
    int c = line.find(":");
    int id = stoi(line.substr(0, c));
    std::string contents = line.substr(c + 2);
    Article* a = new Article(contents, id);

    int indent = i / 2;

    if (indent > lastIndent) {
      parents.push(lastArticle);
    } else if (indent < lastIndent) {
      for (int j = 0; j < (lastIndent - indent); j += 2) {
        parents.pop();
      }
    }

    lastArticle = a;
    lastIndent = indent;
    addArticle(a, parents.top(), id);
  }
}

Article* ArticleManager::getArticle(int id) { root->getChild(id); }

int ArticleManager::addArticle(Article* a, Article* parent, int id) {
  if (!parent) parent = root;
  parent->children.push_back(a);
  a->id = id < 0 ? nextID : id;
  a->parent = parent;
  nextID++;
  return 0;
}

std::string ArticleManager::toString(int count, int offset) {
  if (offset >= nextID) return "";
  if (offset + count >= nextID) count = nextID - offset - 1;
  if (count < 0) return "";

  Article* as[count];
  for (int i = 1; i < nextID; i++) {
    Article* a = getArticle(i);
    int o = a->getOffset() - offset - 1;
    if (o >= 0 && o < count) {
      as[o] = a;
    }
  }

  std::string s;
  for (Article* a : as) {
    int d = a->getDepth() - 1;
    if (a != as[0]) s += "\n";
    for (int i = 0; i < d; i++) {
      s += "    ";
    }
    s += a->toString();
  }
  return s.empty() ? s : s + "\n";
}
