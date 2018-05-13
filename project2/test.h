#pragma once

#include "client.h"
#include "server.h"

void makeServers(Consistency cons);
void makeClients();

int run(int nClnt, int nServ, Consistency cons, float ratio);

int testSeq();
int testRyw();

void teardown();
