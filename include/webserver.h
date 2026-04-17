#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "status.h"

void* start_web_server(void* arg);

typedef struct {
    int port;
    SharedStatus* shared_status;
} WebServerArg;

#endif