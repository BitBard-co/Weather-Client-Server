#include "client.h"
#include "../includes/cities.h"
#include "../includes/city.h"
#include "../includes/utils.h"
#include "../includes/http.h"


void func(int sockfd)
{
    Cities cities;
    cities_init(&cities); // initiera bara en gång

    printf("🌤️  Welcome!\n");
    printf("Please select a city from the list below to get a weather report:\n");

    char buffer[1024];

    while (1) {
        // Visa listan (beroende på hur din Cities-struktur funkar)
        cities_print(&cities); 

        printf("\nEnter city name (or 'quit' to exit): ");
        fgets(buffer, sizeof(buffer), stdin);

        // ta bort newline från fgets
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "quit") == 0) {
            printf("👋 Goodbye!\n");
            break;
        }

        
        write(sockfd, buffer, strlen(buffer));

        bzero(buffer, sizeof(buffer));
        int n = read(sockfd, buffer, sizeof(buffer)-1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Server reply: %s\n\n", buffer);
        } else {
            printf("⚠️  No response from server.\n");
        }
    }
      cities_free(&cities); 
 
}

// Funktion för att starta klienten 
void start_client(void)
{
    int sockfd;
    struct sockaddr_in servaddr;

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

int main(){
    start_client();
    return 0;
}