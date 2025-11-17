#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "weather_server.h"
#include "cities.h"
#include "city.h"
#include "utils.h"
#include "http_server.h"

#define BUFFER_SIZE 8192

void http_server_start(int port) {
    int server_fd;
    struct sockaddr_in addr;

    printf("🌍 HTTP Weather API running on http://localhost:%d\n", port);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        handle_client(client_fd);
        close(client_fd);
    }
}

static void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_fd, buffer, sizeof(buffer)-1);

    if (strncmp(buffer, "GET ", 4) != 0) {
        send_json(client_fd, "{\"error\": \"Only GET supported\"}", 405);
        return;
    }

    if (strncmp(buffer, "GET /weather", 12) == 0) {
        char *city_name = get_query_value(buffer, "city");

        if (!city_name) {
            send_json(client_fd, "{\"error\": \"Missing city parameter\"}", 400);
            return;
        }

        Cities cities;
        cities_init(&cities);

        City *c;
        if (cities_get(&cities, city_name, &c) != 0) {
            if (city_add_from_api(city_name, &cities) != 0) {
                send_json(client_fd, "{\"error\": \"City not found\"}", 404);
                cities_dispose(&cities);
                free(city_name);
                return;
            }
            cities_get(&cities, city_name, &c);
        }

        char *weather_json = weather_server_get_json(c->latitude, c->longitude, c->name);
        send_json(client_fd, weather_json, 200);

        free(weather_json);
        free(city_name);
        cities_dispose(&cities);
        return;
    }

    send_json(client_fd, "{\"error\": \"Not found\"}", 404);
}

static void send_json(int client_fd, const char *json, int status) {
    dprintf(client_fd,
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s\n",
        status, json
    );
}

static char *get_query_value(const char *request, const char *key) {
    char *pos = strstr(request, key);
    if (!pos) return NULL;
    pos += strlen(key);
    if (*pos != '=') return NULL;
    pos++;

    char value[256];
    sscanf(pos, "%255[^& \r]", value);
    return utils_strdup(value);
}
