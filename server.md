// tcp_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>             // read(), write(), close()
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

// ===================================================
// Huvudprogram
// ===================================================
int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // ---------------------------------------------------
    // Skapa socket
    // ---------------------------------------------------
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("[SERVER] Socket successfully created.\n");

    // ---------------------------------------------------
    // Initiera serveradress
    // ---------------------------------------------------
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // ---------------------------------------------------
    // Binda socket till IP och port
    // ---------------------------------------------------
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        perror("Socket bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[SERVER] Socket successfully binded.\n");

    // ---------------------------------------------------
    // Lyssna efter inkommande anslutningar
    // ---------------------------------------------------
    if ((listen(sockfd, 5)) != 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[SERVER] Listening on port %d...\n", PORT);

    // ---------------------------------------------------
    // Acceptera en klient
    // ---------------------------------------------------
    len = sizeof(cli);
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        perror("Server accept failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[SERVER] Client connected.\n");

    // ---------------------------------------------------
    // Kör chattfunktionen
    // ---------------------------------------------------
    chat_with_client(connfd);
    
    // ---------------------------------------------------
    // Stäng socket
    // ---------------------------------------------------
    close(connfd);
    close(sockfd);
    printf("[SERVER] Connection closed.\n");

    return 0;
}

// ===================================================
// Funktion för chatt mellan server och klient
// ===================================================
void chat_with_client(int connfd)
{
    char buff[MAX];
    int n;

    for (;;) {
        bzero(buff, MAX);

        // Läs meddelande från klient
        read(connfd, buff, sizeof(buff));
        printf("From client: %s", buff);

        // Avsluta om klienten säger "exit"
        if (strncmp("exit", buff, 4) == 0) {
            printf("[SERVER] Client disconnected.\n");
            break;
        }

        // Skriv svar till klient
        printf("To client: ");
        bzero(buff, MAX);
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;

        write(connfd, buff, sizeof(buff));

        // Avsluta om servern säger "exit"
        if (strncmp("exit", buff, 4) == 0) {
            printf("[SERVER] Server exiting...\n");
            break;
        }
    }
}
