#include "client.h"
#include "../includes/cities.h"
#include "../includes/city.h"
#include "../includes/utils.h"
#include "../includes/http.h"


// Funktion för kommunikation med servern
void func(int sockfd)
{
  (void)sockfd; // tysta varning
  Cities cities;
  printf("Welcome!\n");
  printf("Please select a city from the list below to get a weather report\n");
  while (1) {

    cities_init(&cities);

    if (utils_break_loop() != 0) {
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

int main(){
    start_client();
    return 0;
}