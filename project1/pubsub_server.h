#pragma once

#include "pubsub.h"

#include "client.h"

char *get_nth_field(char *str, int n, int num_fields = 4, char delim = ';');

int validate(char *article);

int validate_publish(char *article);

int validate_subscribe(char *article);

Client *find_client(char *IP, int port);

bool_t *joinserver_1_svc(char *IP, int ProgID, int ProgVers,
                         struct svc_req *rqstp);

bool_t *leaveserver_1_svc(char *IP, int ProgID, int ProgVers,
                          struct svc_req *rqstp);

bool_t *join_1_svc(char *IP, int Port, struct svc_req *rqstp);

bool_t *leave_1_svc(char *IP, int Port, struct svc_req *rqstp);

bool_t *subscribe_1_svc(char *IP, int Port, char *Article,
                        struct svc_req *rqstp);

bool_t *unsubscribe_1_svc(char *IP, int Port, char *Article,
                          struct svc_req *rqstp);

bool_t *publish_1_svc(char *Article, char *IP, int Port, struct svc_req *rqstp);

bool_t *publishserver_1_svc(char *Article, char *IP, int Port,
                            struct svc_req *rqstp);

bool_t *ping_1_svc(struct svc_req *rqstp);
