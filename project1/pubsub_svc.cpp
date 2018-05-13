/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "pubsub.h"
#include "pubsub_server_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>

static const char *reg_server_name = "dio.cs.umn.edu";
static const int reg_server_port = 5105;
pthread_t server_thread;
pthread_t IO_thread;
struct sockaddr_in server_addr;

int reg_sock;
int ping_sock;
char *ping_ip;
int ping_port;
int client_reg_port;
#define MAXSTRING 120

#ifndef SIG_PF
#define SIG_PF void (*)(int)
#endif

static bool_t *_joinserver_1(joinserver_1_argument *argp,
                             struct svc_req *rqstp) {
  return (joinserver_1_svc(argp->IP, argp->ProgID, argp->ProgVers, rqstp));
}

static bool_t *_leaveserver_1(leaveserver_1_argument *argp,
                              struct svc_req *rqstp) {
  return (leaveserver_1_svc(argp->IP, argp->ProgID, argp->ProgVers, rqstp));
}

static bool_t *_join_1(join_1_argument *argp, struct svc_req *rqstp) {
  return (join_1_svc(argp->IP, argp->Port, rqstp));
}

static bool_t *_leave_1(leave_1_argument *argp, struct svc_req *rqstp) {
  return (leave_1_svc(argp->IP, argp->Port, rqstp));
}

static bool_t *_subscribe_1(subscribe_1_argument *argp, struct svc_req *rqstp) {
  return (subscribe_1_svc(argp->IP, argp->Port, argp->Article, rqstp));
}

static bool_t *_unsubscribe_1(unsubscribe_1_argument *argp,
                              struct svc_req *rqstp) {
  return (unsubscribe_1_svc(argp->IP, argp->Port, argp->Article, rqstp));
}

static bool_t *_publish_1(publish_1_argument *argp, struct svc_req *rqstp) {
  return (publish_1_svc(argp->Article, argp->IP, argp->Port, rqstp));
}

static bool_t *_publishserver_1(publishserver_1_argument *argp,
                                struct svc_req *rqstp) {
  return (publishserver_1_svc(argp->Article, argp->IP, argp->Port, rqstp));
}

static bool_t *_ping_1(void *argp, struct svc_req *rqstp) {
  return (ping_1_svc(rqstp));
}

static void pubsub_prog_1(struct svc_req *rqstp, register SVCXPRT *transp) {
  union {
    joinserver_1_argument joinserver_1_arg;
    leaveserver_1_argument leaveserver_1_arg;
    join_1_argument join_1_arg;
    leave_1_argument leave_1_arg;
    subscribe_1_argument subscribe_1_arg;
    unsubscribe_1_argument unsubscribe_1_arg;
    publish_1_argument publish_1_arg;
    publishserver_1_argument publishserver_1_arg;
  } argument;
  char *result;
  xdrproc_t _xdr_argument, _xdr_result;
  char *(*local)(char *, struct svc_req *);

  switch (rqstp->rq_proc) {
    case NULLPROC:
      (void)svc_sendreply(transp, (xdrproc_t)xdr_void, (char *)NULL);
      return;

    case JoinServer:
      _xdr_argument = (xdrproc_t)xdr_joinserver_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_joinserver_1;
      break;

    case LeaveServer:
      _xdr_argument = (xdrproc_t)xdr_leaveserver_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_leaveserver_1;
      break;

    case Join:
      _xdr_argument = (xdrproc_t)xdr_join_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_join_1;
      break;

    case Leave:
      _xdr_argument = (xdrproc_t)xdr_leave_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_leave_1;
      break;

    case Subscribe:
      _xdr_argument = (xdrproc_t)xdr_subscribe_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_subscribe_1;
      break;

    case Unsubscribe:
      _xdr_argument = (xdrproc_t)xdr_unsubscribe_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_unsubscribe_1;
      break;

    case Publish:
      _xdr_argument = (xdrproc_t)xdr_publish_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_publish_1;
      break;

    case PublishServer:
      _xdr_argument = (xdrproc_t)xdr_publishserver_1_argument;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_publishserver_1;
      break;

    case Ping:
      _xdr_argument = (xdrproc_t)xdr_void;
      _xdr_result = (xdrproc_t)xdr_bool;
      local = (char *(*)(char *, struct svc_req *))_ping_1;
      break;

    default:
      svcerr_noproc(transp);
      return;
  }
  memset((char *)&argument, 0, sizeof(argument));
  if (!svc_getargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
    svcerr_decode(transp);
    return;
  }
  result = (*local)((char *)&argument, rqstp);
  if (result != NULL &&
      !svc_sendreply(transp, (xdrproc_t)_xdr_result, result)) {
    svcerr_systemerr(transp);
  }
  if (!svc_freeargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
    fprintf(stderr, "%s", "unable to free arguments");
    exit(1);
  }
  return;
}

void *handle_input(void *arg) {
  std::string s;
  while (true) {
    std::cin >> s;
    char message[MAXSTRING];
    if (s.compare("get_list") == 0) {
      sprintf(message, "GetList;RPC;%s;%d", ping_ip, ping_port);
      sendto(reg_sock, message, sizeof(message), 0,
             (struct sockaddr *)&server_addr, sizeof(server_addr));
      char list[1024];
      recvfrom(reg_sock, list, sizeof(list), 0, NULL, NULL);
      printf("List: %s\n", list);
    }
  }
}

void Deregister() {
  fprintf(stderr, "closing connection\n");
  char message[MAXSTRING];
  sprintf(message, "Deregister;RPC;%s;%d", ping_ip, ping_port);
  sendto(ping_sock, message, sizeof(message), 0,
         (struct sockaddr *)&server_addr, sizeof(server_addr));

  close(ping_sock);
  close(reg_sock);
}

void *Register(void *arg) {
  if ((reg_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    exit(1);
  }

  struct hostent *hp = gethostbyname(reg_server_name);
  if (!hp) {
    fprintf(stderr, "could not obtain server address (%s)\n", reg_server_name);
    exit(1);
  }
  memset((char *)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(reg_server_port);
  memcpy((void *)&server_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

  socklen_t reg_len = sizeof(server_addr);
  if (connect(reg_sock, (struct sockaddr *)&server_addr, reg_len) < 0) {
    perror("connect");
    exit(1);
  }

  struct sockaddr_in ping_addr;
  socklen_t ping_len = sizeof(ping_addr);
  if (getsockname(reg_sock, (struct sockaddr *)&ping_addr, &ping_len) < 0) {
    perror("getsockname failed");
    exit(1);
  }

  client_reg_port = ntohs(ping_addr.sin_port);
  ping_ip = inet_ntoa(ping_addr.sin_addr);

  if ((ping_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    exit(1);
  }

  ping_addr.sin_port = htons(0);

  if (bind(ping_sock, (struct sockaddr *)&ping_addr, sizeof(ping_addr)) < 0) {
    perror("bind failed");
    exit(1);
  }

  if (getsockname(ping_sock, (struct sockaddr *)&ping_addr, &ping_len) < 0) {
    perror("getsockname");
    exit(1);
  }

  ping_port = ntohs(ping_addr.sin_port);

  char message[MAXSTRING];
  sprintf(message, "Register;RPC;%s;%d;%x;%d", ping_ip, ping_port, PUBSUB_PROG,
          PUBSUB_VERSION);
  sendto(reg_sock, message, sizeof(message), 0, (struct sockaddr *)&server_addr,
         sizeof(server_addr));

  if (pthread_create(&IO_thread, NULL, handle_input, NULL)) {
    fprintf(stderr, "Error creating I/O thread\n");
    exit(1);
  }

  char heartbeat[1024];
  struct sockaddr_in server_recvd;
  socklen_t recvd_len = sizeof(server_recvd);
  char *response = "heartbeat";
  while (1) {
    recvfrom(ping_sock, heartbeat, sizeof(heartbeat), 0,
             (struct sockaddr *)&server_recvd, &recvd_len);
    // printf("%s\n", heartbeat);
    sendto(ping_sock, response, sizeof(response), 0,
           (struct sockaddr *)&server_recvd, recvd_len);
  }
}

void sever_terminate(int signum) {
  fprintf(stderr, "server terminating\n");
  Deregister();
  exit(signum);
}

int main(int argc, char **argv) {
  register SVCXPRT *transp;

  pmap_unset(PUBSUB_PROG, PUBSUB_VERSION);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, PUBSUB_PROG, PUBSUB_VERSION, pubsub_prog_1,
                    IPPROTO_UDP)) {
    fprintf(stderr, "%s",
            "unable to register (PUBSUB_PROG, PUBSUB_VERSION, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, PUBSUB_PROG, PUBSUB_VERSION, pubsub_prog_1,
                    IPPROTO_TCP)) {
    fprintf(stderr, "%s",
            "unable to register (PUBSUB_PROG, PUBSUB_VERSION, tcp).");
    exit(1);
  }
  signal(SIGINT, sever_terminate);

  if (argc > 1 && !strncmp(argv[1], "test", 4)) {
    printf("Running server tests\n");
    test_main();
    exit(0);
  } else {
    if (pthread_create(&server_thread, NULL, Register, NULL)) {
      fprintf(stderr, "Error creating pinging thread\n");
      exit(1);
    }

    printf("Ready to handle requests\n");

    svc_run();
    fprintf(stderr, "%s", "svc_run returned");
    exit(1);
  }

  /* NOTREACHED */
}
