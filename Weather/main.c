#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "includes/cities.h"
#include "includes/meteo.h"
#include "includes/city.h"
#include "includes/utils.h"
#include "includes/networkhandler.h"
#include "includes/http.h"

static int is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// If the payload looks like HTTP chunked transfer framing, decode it for display/parsing.
// Returns newly allocated buffer (caller frees) or NULL if decoding fails.
static char* try_decode_http_chunked(const char* in)
{
    if (in == NULL) return NULL;

    const char* p = in;
    while (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t') p++;

    // Heuristic: must start with hex digits then CRLF.
    if (!is_hex_digit(*p)) return NULL;
    const char* first_crlf = strstr(p, "\r\n");
    if (first_crlf == NULL) return NULL;
    for (const char* t = p; t < first_crlf; t++) {
        if (*t == ';') break; // chunk extensions
        if (!is_hex_digit(*t)) return NULL;
    }

    size_t in_len = strlen(in);
    char* out = (char*)malloc(in_len + 1);
    if (out == NULL) return NULL;
    size_t out_len = 0;

    while (1) {
        // Read chunk size line
        char* endptr = NULL;
        unsigned long chunk_size = strtoul(p, &endptr, 16);
        if (endptr == (char*)p) {
            free(out);
            return NULL;
        }
        const char* line_end = strstr(p, "\r\n");
        if (line_end == NULL) {
            free(out);
            return NULL;
        }
        p = line_end + 2;

        if (chunk_size == 0) {
            // Consume trailing headers until final CRLFCRLF (optional)
            break;
        }

        // Copy chunk data
        for (unsigned long i = 0; i < chunk_size; i++) {
            if (p[i] == '\0') {
                free(out);
                return NULL;
            }
        }
        memcpy(out + out_len, p, (size_t)chunk_size);
        out_len += (size_t)chunk_size;
        p += chunk_size;

        // Expect CRLF after chunk data
        if (p[0] != '\r' || p[1] != '\n') {
            free(out);
            return NULL;
        }
        p += 2;
    }

    out[out_len] = '\0';
    return out;
}

static void print_weather_pretty(const char* city_name, const char* json)
{
    if (json == NULL || json[0] == '\0') {
        printf("\n(no weather data)\n");
        return;
    }

    // Some upstream responses may be cached/stored with HTTP chunked framing.
    // We only decode for display/parsing (does not change server behavior).
    char* decoded = NULL;
    const char* to_parse = json;

    cJSON* root = cJSON_Parse(to_parse);
    if (root == NULL || !cJSON_IsObject(root)) {
        if (root != NULL) cJSON_Delete(root);
        decoded = try_decode_http_chunked(json);
        if (decoded != NULL) {
            to_parse = decoded;
            root = cJSON_Parse(to_parse);
        }
    }

    if (root == NULL || !cJSON_IsObject(root)) {
        if (root != NULL) cJSON_Delete(root);
        if (decoded != NULL) free(decoded);
        printf("\n(raw server response)\n%s\n", json);
        return;
    }

    const cJSON* latitude = cJSON_GetObjectItemCaseSensitive(root, "latitude");
    const cJSON* longitude = cJSON_GetObjectItemCaseSensitive(root, "longitude");
    const cJSON* timezone = cJSON_GetObjectItemCaseSensitive(root, "timezone");
    const cJSON* current = cJSON_GetObjectItemCaseSensitive(root, "current_weather");

    const cJSON* temp = current ? cJSON_GetObjectItemCaseSensitive(current, "temperature") : NULL;
    const cJSON* windspeed = current ? cJSON_GetObjectItemCaseSensitive(current, "windspeed") : NULL;
    const cJSON* winddir = current ? cJSON_GetObjectItemCaseSensitive(current, "winddirection") : NULL;
    const cJSON* weathercode = current ? cJSON_GetObjectItemCaseSensitive(current, "weathercode") : NULL;
    const cJSON* time = current ? cJSON_GetObjectItemCaseSensitive(current, "time") : NULL;

    printf("\nWeather summary\n");
    printf("\n");
    printf("📍 City: %s\n", city_name ? city_name : "(unknown)");
    if (cJSON_IsNumber(latitude) && cJSON_IsNumber(longitude)) {
        printf("🧭 Coords: %.4f, %.4f\n", latitude->valuedouble, longitude->valuedouble);
    }
    if (cJSON_IsString(timezone) && timezone->valuestring) {
        printf("🗺️  Timezone: %s\n", timezone->valuestring);
    }
    if (cJSON_IsString(time) && time->valuestring) {
        printf("🕒 Time: %s\n", time->valuestring);
    }
    if (cJSON_IsNumber(temp)) {
        printf("🌡️  Temperature: %.1f °C\n", temp->valuedouble);
    }
    if (cJSON_IsNumber(windspeed) || cJSON_IsNumber(winddir)) {
        printf("💨 Wind: ");
        if (cJSON_IsNumber(windspeed)) printf("%.1f km/h", windspeed->valuedouble);
        if (cJSON_IsNumber(winddir)) printf(" @ %.0f°", winddir->valuedouble);
        printf("\n");
    }
    if (cJSON_IsNumber(weathercode)) {
        printf("🌦️  Weather code: %d\n", weathercode->valueint);
    }

    printf("\nMore? Type 'm' + Enter to show raw JSON, or just press Enter to skip: ");
    char choice[8];
    if (fgets(choice, sizeof(choice), stdin) != NULL && (choice[0] == 'm' || choice[0] == 'M')) {
        char* pretty = cJSON_Print(root);
        if (pretty != NULL) {
            printf("\nRaw JSON\n%s\n", pretty);
            free(pretty);
        }
    }

    cJSON_Delete(root);
    if (decoded != NULL) free(decoded);
}

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
                            
                            // Query local HTTP server for weather data
                            char url[256];
                            snprintf(url, sizeof(url), "http://127.0.0.1:8080/weather?city=%s", selected_city->name);
                            NetworkHandler* nh = NULL;
                            int rc = http_api_request(url, &nh);
                            if (rc == 0 && nh && nh->data) {
                                print_weather_pretty(selected_city->name, nh->data);
                                networkhandler_dispose(nh);
                            } else {
                                printf("Failed to retrieve weather from server. Is it running? rc=%d\n", rc);
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