// tcp_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>             // read(), write(), close()
#include <arpa/inet.h>          // inet_addr()
#include <netdb.h>
#include <sys/socket.h>

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

// ===================================================
// Funktionsdeklaration
// ===================================================
void chat_with_server(int sockfd);

// ===================================================
// Huvudprogram
// ===================================================
int main()
{
    int sockfd;
    struct sockaddr_in servaddr;

    // ---------------------------------------------------
    // Skapa socket
    // ---------------------------------------------------
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("[CLIENT] Socket successfully created.\n");

    // ---------------------------------------------------
    // Initiera serveradress
    // ---------------------------------------------------
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // ---------------------------------------------------
    // Anslut till servern
    // ---------------------------------------------------
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        perror("Connection to server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[CLIENT] Connected to the server.\n");

    // ---------------------------------------------------
    // Kör chattfunktion
    // ---------------------------------------------------
    chat_with_server(sockfd);

    // ---------------------------------------------------
    // Stäng socket
    // ---------------------------------------------------
    close(sockfd);
    printf("[CLIENT] Connection closed.\n");

    return 0;
}

// ===================================================
// Funktion för chatt mellan klient och server
// ===================================================
void chat_with_server(int sockfd)
{
    char buff[MAX];
    int n;

    for (;;) {
        bzero(buff, sizeof(buff));

        // Läs input från användaren
        printf("Enter message: ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;

        // Skicka till servern
        write(sockfd, buff, sizeof(buff));

        // Läs svar från servern
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server: %s", buff);

        // Om "exit" skrivs avslutas chatten
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("[CLIENT] Exiting chat...\n");
            break;
        }
    }
}
