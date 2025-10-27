#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h>

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

// Funktioner
void func(int sockfd);
void start_client(void);
#endif // CLIENT_H
