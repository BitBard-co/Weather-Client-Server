#include "client.h"

// Funktion för kommunikation med servern
void func(int sockfd)
{
    char buff[MAX];
    int n;

    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;

        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);

        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}

// Funktion för att starta klienten (istället för main)
void start_client(void)
{
    int sockfd;
    struct sockaddr_in servaddr;

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
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // Anslut till server
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("❌ Connection with the server failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("✅ Connected to the server.\n");

    // Kör kommunikationsloopen
    func(sockfd);

    // Stäng socket
    close(sockfd);
}
