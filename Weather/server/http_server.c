#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../includes/cities.h"
#include "../includes/city.h"
#include "../includes/utils.h"
#include "../includes/networkhandler.h"
#include "../includes/meteo.h"
#include "../src/libs/cJSON/cJSON.h"

#define HTTP_PORT 8080
#define REQ_BUF_SIZE 8192

static void send_response(int fd, int status, const char* status_text, const char* content_type, const char* body) {
    char header[512];
    size_t body_len = body ? strlen(body) : 0;
    int n = snprintf(header, sizeof(header),
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     status, status_text, content_type, body_len);
    send(fd, header, n, 0);
    if (body && body_len > 0) {
        send(fd, body, body_len, 0);
    }
}

static int urldecode_inplace(char* s) {
    char *o = s, *p = s;
    while (*p) {
        if (*p == '+') { *o++ = ' '; p++; }
        else if (*p == '%' && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2])) {
            char hex[3] = { p[1], p[2], '\0' };
            *o++ = (char)strtol(hex, NULL, 16);
            p += 3;
        } else {
            *o++ = *p++;
        }
    }
    *o = '\0';
    return 0;
}

static const char* get_query_param(const char* query, const char* key, char* out, size_t out_sz) {
    if (!query || !key || !out || out_sz == 0) return NULL;
    size_t keylen = strlen(key);
    const char* p = query;
    while (p && *p) {
        const char* amp = strchr(p, '&');
        size_t len = amp ? (size_t)(amp - p) : strlen(p);
        if (len >= keylen + 1 && strncmp(p, key, keylen) == 0 && p[keylen] == '=') {
            size_t vlen = len - (keylen + 1);
            if (vlen >= out_sz) vlen = out_sz - 1;
            memcpy(out, p + keylen + 1, vlen);
            out[vlen] = '\0';
            urldecode_inplace(out);
            return out;
        }
        p = amp ? amp + 1 : NULL;
    }
    return NULL;
}

static void handle_health(int fd) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    char* json = cJSON_PrintUnformatted(root);
    send_response(fd, 200, "OK", "application/json; charset=utf-8", json);
    cJSON_free(json);
    cJSON_Delete(root);
}

static void handle_cities(int fd) {
    Cities cities = {0};
    if (cities_init(&cities) != 0) {
        send_response(fd, 500, "Internal Server Error", "application/json", "{\"error\":\"cities_init failed\"}");
        return;
    }
    cJSON* arr = cJSON_CreateArray();
    if (cities.head) {
        City* cur = cities.head;
        while (cur) {
            cJSON_AddItemToArray(arr, cJSON_CreateString(cur->name ? cur->name : ""));
            cur = cur->next;
        }
    }
    char* json = cJSON_PrintUnformatted(arr);
    send_response(fd, 200, "OK", "application/json; charset=utf-8", json);
    cJSON_free(json);
    cJSON_Delete(arr);
    cities_dispose(&cities);
}

static void handle_weather(int fd, const char* query) {
    char city_param[256] = {0};
    if (!get_query_param(query, "city", city_param, sizeof(city_param))) {
        send_response(fd, 400, "Bad Request", "application/json", "{\"error\":\"missing city query param\"}");
        return;
    }

    Cities cities = {0};
    if (cities_init(&cities) != 0) {
        send_response(fd, 500, "Internal Server Error", "application/json", "{\"error\":\"cities_init failed\"}");
        return;
    }

    City* found = NULL;
    if (cities_get(&cities, city_param, &found) != 0) {
        /* Try add via API then look again */
        if (city_add_from_api(city_param, &cities) != 0 || cities_get(&cities, city_param, &found) != 0) {
            cities_dispose(&cities);
            send_response(fd, 404, "Not Found", "application/json", "{\"error\":\"city not found\"}");
            return;
        }
    }

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current_weather=true",
             (double)found->latitude, (double)found->longitude);

    Meteo* m = NULL;
    if (networkhandler_get_data(url, &m, FLAG_WRITE) != 0 || !m || !m->data) {
        if (m) { free(m->data); free(m); }
        cities_dispose(&cities);
        send_response(fd, 502, "Bad Gateway", "application/json", "{\"error\":\"failed to fetch weather\"}");
        return;
    }

    /* Enrich with city name? Keep original JSON to stay faithful */
    send_response(fd, 200, "OK", "application/json; charset=utf-8", m->data);

    free(m->data);
    free(m);
    cities_dispose(&cities);
}

static void handle_connection(int fd) {
    char req[REQ_BUF_SIZE];
    ssize_t r = recv(fd, req, sizeof(req) - 1, 0);
    if (r <= 0) {
        return;
    }
    req[r] = '\0';

    /* Parse request line */
    char method[8] = {0}, target[1024] = {0};
    if (sscanf(req, "%7s %1023s", method, target) != 2) {
        send_response(fd, 400, "Bad Request", "text/plain", "Bad Request");
        return;
    }
    if (strcmp(method, "GET") != 0) {
        send_response(fd, 405, "Method Not Allowed", "text/plain", "Only GET supported");
        return;
    }

    /* Split path and query */
    char path[1024] = {0};
    char* qmark = strchr(target, '?');
    if (qmark) {
        size_t len = (size_t)(qmark - target);
        if (len >= sizeof(path)) len = sizeof(path) - 1;
        memcpy(path, target, len); path[len] = '\0';
    } else {
        snprintf(path, sizeof(path), "%s", target);
    }
    const char* query = qmark ? (qmark + 1) : NULL;

    if (strcmp(path, "/health") == 0) {
        handle_health(fd);
    } else if (strcmp(path, "/cities") == 0) {
        handle_cities(fd);
    } else if (strcmp(path, "/weather") == 0) {
        handle_weather(fd, query);
    } else {
        send_response(fd, 404, "Not Found", "application/json", "{\"error\":\"not found\"}");
    }
}

int main(int argc, char** argv) {
    int port = HTTP_PORT;
    if (argc >= 2) {
        port = atoi(argv[1]);
        if (port <= 0) port = HTTP_PORT;
    }

    /* Ensure relative paths match repo root expectations if run from server/ */
    /* Optional: try chdir("..") if cache/cities not found here */
    /* utils_create_folder will create if missing; no fatal if separate */

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s);
        return 1;
    }
    if (listen(s, 16) < 0) {
        perror("listen");
        close(s);
        return 1;
    }
    printf("HTTP server listening on port %d\n", port);

    for (;;) {
        struct sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int cfd = accept(s, (struct sockaddr*)&cli, &clen);
        if (cfd < 0) { if (errno == EINTR) continue; perror("accept"); break; }
        handle_connection(cfd);
        close(cfd);
    }

    close(s);
    return 0;
}
