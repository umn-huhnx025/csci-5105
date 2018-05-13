/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "pubsub.h"
#include "pubsub_server.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <string>

#include "client.h"

std::vector<Client *> clients;

char *get_nth_field(char *str, int n, int num_fields, char delim) {
  char *i;
  char *sep[num_fields + 1];
  sep[0] = str;
  sep[4] = &str[strlen(str)] + 1;
  for (int i = 1; i < num_fields; i++) {
    sep[i] = strchr(sep[i - 1], delim);
    if (!sep[i]) return NULL;
    sep[i]++;
  }
  int result_len = sep[n + 1] - sep[n] - 1;
  char *result = (char *)malloc(result_len + 1);
  memcpy(result, sep[n], result_len);
  result[result_len] = '\0';
  return result;
}

int validate(char *article) {
  if (strlen(article) > MAXSTRING) {
    return 0;
  }

  char *i0 = get_nth_field(article, 0);
  char *i1 = get_nth_field(article, 1);
  char *i2 = get_nth_field(article, 2);
  char *i3 = get_nth_field(article, 3);
  int result = i0 && !(!*i0 && !*i1 && !*i2 && *i3);

  free(i0);
  free(i1);
  free(i2);
  free(i3);

  return result;
}

int validate_publish(char *article) {
  if (!validate(article)) return 0;

  char *type = get_nth_field(article, 0);
  if (*type) {
    auto found = Client::article_types.find(type);
    if (found == Client::article_types.end()) return 0;
  }

  char *contents = get_nth_field(article, 3);
  int result = contents && *contents;
  free(contents);
  return result;
}

int validate_subscribe(char *article) {
  if (!validate(article)) return 0;

  char *i0 = get_nth_field(article, 0);
  char *i1 = get_nth_field(article, 1);
  char *i2 = get_nth_field(article, 2);
  char *i3 = get_nth_field(article, 3);

  int result = i0 && (*i0 || *i1 || *i2) && !*i3;

  if (*i0) {
    auto found = Client::article_types.find(i0);
    if (found == Client::article_types.end()) return 0;
  }

  free(i0);
  free(i1);
  free(i2);
  free(i3);

  return result;
}

Client *find_client(char *IP, int port) {
  int ip;
  inet_pton(AF_INET, IP, &ip);
  for (Client *c : clients) {
    if (c->ip == ip && c->port == port) {
      return c;
    }
  }
  return nullptr;
}

bool_t *joinserver_1_svc(char *IP, int ProgID, int ProgVers,
                         struct svc_req *rqstp) {
  static bool_t result;

  /*
   * insert server code here
   */

  return &result;
}

bool_t *leaveserver_1_svc(char *IP, int ProgID, int ProgVers,
                          struct svc_req *rqstp) {
  static bool_t result;

  /*
   * insert server code here
   */

  return &result;
}

bool_t *join_1_svc(char *IP, int Port, struct svc_req *rqstp) {
  static bool_t result;

  if (clients.size() >= MAXCLIENT) {
    fprintf(stderr, "No room for more clients\n");
    result = 1;
    return &result;
  }

  // Check if this client is already registered
  if (find_client(IP, Port)) {
    fprintf(stderr, "Client %s:%d is already registered with the server\n", IP,
            Port);
    result = 1;
    return &result;
  }

  // Put this client in the first free slot in our registry
  clients.push_back(new Client(IP, Port));
  printf("Successfully registered %s:%d\n", IP, Port);

  result = 0;
  return &result;
}

bool_t *leave_1_svc(char *IP, int Port, struct svc_req *rqstp) {
  static bool_t result;

  // Search the registry for a matching ip/port pair
  Client *client_to_delete = find_client(IP, Port);
  if (!client_to_delete) {
    fprintf(stderr, "Client %s:%d not found\n", IP, Port);
    result = 1;
    return &result;
  }

  delete client_to_delete;
  clients.erase(std::remove(clients.begin(), clients.end(), client_to_delete));
  printf("Successfully unregistered %s:%d\n", IP, Port);
  result = 0;
  return &result;
}

bool_t *subscribe_1_svc(char *IP, int Port, char *Article,
                        struct svc_req *rqstp) {
  static bool_t result;

  if (!validate_subscribe(Article)) {
    fprintf(stderr, "Invalid subscription format: '%s'\n", Article);
    result = 1;
    return &result;
  }

  Client *c = find_client(IP, Port);
  if (!c) {
    fprintf(stderr, "Could not find client %s:%d\n", IP, Port);
    result = 1;
    return &result;
  }

  for (auto sub : c->subs) {
    if (!strncmp(sub.c_str(), Article, strlen(Article))) {
      fprintf(stderr, "%s:%d is already subscribed to '%s'\n", IP, Port,
              Article);
      result = 0;
      return &result;
    }
  }

  c->subs.push_back(std::string(Article));
  printf("%s:%d subscribed to: '%s'\n", IP, Port, Article);

  result = 0;
  return &result;
}

bool_t *unsubscribe_1_svc(char *IP, int Port, char *Article,
                          struct svc_req *rqstp) {
  static bool_t result;

  if (!validate_subscribe(Article)) {
    fprintf(stderr, "Invalid subscription format: '%s'\n", Article);
    result = 1;
    return &result;
  }

  Client *c = find_client(IP, Port);
  if (!c) {
    fprintf(stderr, "Could not find client %s:%d\n", IP, Port);
    result = 1;
    return &result;
  }
  int i = 0;
  for (auto sub : c->subs) {
    if (!strncmp(sub.c_str(), Article, strlen(Article))) {
      c->subs.erase(c->subs.begin() + i);
      printf("%s:%d unsubscribed from: '%s'\n", IP, Port, Article);
      result = 0;
      return &result;
    }
    i++;
  }

  result = 1;
  return &result;
}

bool_t *publish_1_svc(char *Article, char *IP, int Port,
                      struct svc_req *rqstp) {
  static bool_t result = 0;

  if (!validate_publish(Article)) {
    fprintf(stderr, "Invalid article format: '%s'\n", Article);
    result = 1;
    return &result;
  }

  printf("Published new article: '%s'\n", Article);
  result = 0;

  // Check subscriptions
  for (Client *c : clients) {
    if (c->is_subscribed(Article)) {
      result |= c->send(Article);
    }
  }

  return &result;
}

bool_t *publishserver_1_svc(char *Article, char *IP, int Port,
                            struct svc_req *rqstp) {
  static bool_t result;

  /*
   * insert server code here
   */

  return &result;
}

bool_t *ping_1_svc(struct svc_req *rqstp) {
  static bool_t result;

  // printf("pinged\n");

  return &result;
}
