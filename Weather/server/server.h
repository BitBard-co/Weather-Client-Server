#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "../includes/meteo.h"
#include "../includes/cities.h"
#include "../includes/city.h"
#include "../includes/parsedata.h"
#include "../includes/networkhandler.h"
#include "../includes/utils.h"

#define MAX 80
#define PORT 8080
#define SA struct sockaddr
#define MAX 1024

// Funktioner
void func_server(int connfd);
void start_server(void);

#endif // SERVER_H
