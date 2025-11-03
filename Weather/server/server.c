#include "server.h"

// Funktion för chatten mellan klient och server
void func_server(int connfd)
{
    char buff[MAX];
    int n;

    for (;;) {
        bzero(buff, MAX);
        read(connfd, buff, sizeof(buff));
        printf("From client: %s\tTo client: ", buff);

        bzero(buff, MAX);
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;

        write(connfd, buff, sizeof(buff));

        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}

// Funktion för att starta servern
void start_server(void)
{
    int sockfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;

    // Skapa socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("❌ Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("✅ Socket successfully created.\n");

    bzero(&servaddr, sizeof(servaddr));

    // Tilldela IP och port
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Bind socket till IP och port
    if (bind(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("❌ Socket bind failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("✅ Socket successfully binded.\n");

    // Starta lyssning
    if (listen(sockfd, 5) != 0) {
        printf("❌ Listen failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("✅ Server listening...\n");

    len = sizeof(cli);

    // Acceptera klient
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("❌ Server accept failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("✅ Server accepted the client.\n");

    // Kör chatfunktion
    func_server(connfd);

    // Stäng socket
    close(sockfd);
}

int main(void) {
    start_server();
    return 0;
}
