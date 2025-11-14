#include "server.h"

void faktan(const char *cityName, char *response, size_t size) {
    printf("🌤️  Client requested weather for: %s\n", cityName);

    snprintf(response, size,
             "✅ Got your request for '%s' — server is processing it!\n",
             cityName);
}

void func_server(int connfd)
{
    char cityName[MAX];
    char response[MAX];

    for (;;) {
        bzero(cityName, sizeof(cityName));

        int n = read(connfd, cityName, sizeof(cityName) - 1);
        if (n <= 0)
            break; // client disconnected

        // Remove newline if present
        cityName[strcspn(cityName, "\n")] = '\0';

        // Exit check
        if (strncmp(cityName, "exit", 4) == 0) {
            printf("Server Exit...\n");
            break;
        }

        // Generate the response using helper
        faktan(cityName, response, sizeof(response));

        // Send it to the client
        write(connfd, response, strlen(response));
    }

    close(connfd);
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
