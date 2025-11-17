
#ifndef SERVER_H
#define SERVER_H

#define MAX 80
#define PORT 8080
#define SA struct sockaddr
#define MAX2 1024   

// Funktioner
void func_server(int connfd);
void start_server(void);

#endif // SERVER_H
