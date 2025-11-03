#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "includes/cities.h"
#include "includes/meteo.h"
#include "includes/city.h"
#include "includes/utils.h"

int main() {
    printf("=== Weather CLI Application ===\n");
    printf("Welcome to the Weather Client-Server Application!\n\n");
    
    Cities cities = {0};
    
    // Initialize cities list
    if (cities_init(&cities) != 0) {
        printf("Failed to initialize cities database.\n");
        return EXIT_FAILURE;
    }
    
    char input[256];
    int choice;
    
    while (1) {
        printf("\n=== Available Cities ===\n");
        cities_print(&cities);
        
        printf("Options:\n");
        printf("1. Get weather for a city (by name)\n");
        printf("2. Search for a new city\n");
        printf("3. Exit\n");
        printf("\nEnter your choice: ");
        
        if (fgets(input, sizeof(input), stdin) != NULL) {
            choice = atoi(input);
            
            switch (choice) {
                case 1: {
                    printf("Enter city name: ");
                    if (fgets(input, sizeof(input), stdin) != NULL) {
                        // Remove newline
                        input[strcspn(input, "\n")] = 0;
                        
                        City* selected_city = NULL;
                        if (cities_get(&cities, input, &selected_city) == 0 && selected_city != NULL) {
                            printf("\nFetching weather for %s...\n", selected_city->name);
                            
                            if (meteo_get_weather_data(selected_city->latitude, 
                                                     selected_city->longitude, 
                                                     selected_city->name) == 0) {
                                printf("Weather data retrieved successfully!\n");
                            } else {
                                printf("Failed to retrieve weather data.\n");
                            }
                        } else {
                            printf("City not found. Try option 2 to search for it.\n");
                        }
                    }
                    break;
                }
                case 2: {
                    printf("Enter city name to search: ");
                    if (fgets(input, sizeof(input), stdin) != NULL) {
                        // Remove newline
                        input[strcspn(input, "\n")] = 0;
                        
                        printf("Searching for city: %s\n", input);
                        cJSON* city_data = meteo_get_city_data(input);
                        if (city_data) {
                            printf("City found and added to database!\n");
                            cJSON_Delete(city_data);
                            
                            // Reinitialize cities to include the new one
                            cities_dispose(&cities);
                            if (cities_init(&cities) != 0) {
                                printf("Failed to reinitialize cities database.\n");
                                return EXIT_FAILURE;
                            }
                        } else {
                            printf("City not found or error occurred.\n");
                        }
                    }
                    break;
                }
                case 3:
                    printf("Goodbye!\n");
                    cities_dispose(&cities);
                    return EXIT_SUCCESS;
                    
                default:
                    printf("Invalid choice. Please try again.\n");
                    break;
            }
        }
        
        printf("\nPress Enter to continue...");
        getchar();
    }
    
    cities_dispose(&cities);
    return EXIT_SUCCESS;
}