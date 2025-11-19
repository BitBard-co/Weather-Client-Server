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

#define HTTP_PORT 22 /* Default now set to 22 to match router forward (external 10722 -> internal 22) */
#define REQ_BUF_SIZE 8192

typedef struct {
    char addr[64];
    int port;
} ServerConfig;

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

static void trim_inplace(char* s) {
    if (!s) return;
    char* start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    char* end = s + strlen(s);
    while (end > start && isspace((unsigned char)end[-1])) --end;
    size_t len = (size_t)(end - start);
    if (start != s) memmove(s, start, len);
    s[len] = '\0';
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
    trim_inplace(city_param);
    /* Debug: show raw incoming city parameter */
    fprintf(stderr, "[weather] incoming city raw='%s'\n", city_param);

    Cities cities = {0};
    if (cities_init(&cities) != 0) {
        send_response(fd, 500, "Internal Server Error", "application/json", "{\"error\":\"cities_init failed\"}");
        return;
    }

    City* found = NULL;
    int first_lookup = cities_get(&cities, city_param, &found);
    if (first_lookup != 0) {
        fprintf(stderr, "[weather] initial lookup failed for '%s' -> attempting API add\n", city_param);
        int add_rc = city_add_from_api(city_param, &cities);
        fprintf(stderr, "[weather] city_add_from_api rc=%d for '%s'\n", add_rc, city_param);
        int second_lookup = cities_get(&cities, city_param, &found);
        fprintf(stderr, "[weather] second lookup rc=%d for '%s'\n", second_lookup, city_param);
        if (add_rc != 0 || second_lookup != 0) {
            cities_dispose(&cities);
            send_response(fd, 404, "Not Found", "application/json", "{\"error\":\"city not found\"}");
            return;
        }
    } else {
        fprintf(stderr, "[weather] found existing city '%s' (lat=%.4f lon=%.4f)\n", found->name, (double)found->latitude, (double)found->longitude);
    }

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current_weather=true",
             (double)found->latitude, (double)found->longitude);

    Meteo* m = NULL;
    int net_rc = networkhandler_get_data(url, &m, FLAG_WRITE);
    fprintf(stderr, "[weather] networkhandler_get_data rc=%d url='%s'\n", net_rc, url);
    if (net_rc != 0 || !m || !m->data) {
        if (m) { free(m->data); free(m); }
        cities_dispose(&cities);
        send_response(fd, 502, "Bad Gateway", "application/json", "{\"error\":\"failed to fetch weather\"}");
        return;
    }

    /* Enrich with city name? Keep original JSON to stay faithful */
    send_response(fd, 200, "OK", "application/json; charset=utf-8", m->data);
    fprintf(stderr, "[weather] served weather for '%s' (%s)\n", found->name, city_param);

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
    char* header_end = strstr(req, "\r\n\r\n");
    size_t header_len = header_end ? (size_t)(header_end - req) : (size_t)r;
    char* body_ptr = header_end ? (header_end + 4) : NULL;

    char method[8] = {0}, target[1024] = {0};
    if (sscanf(req, "%7s %1023s", method, target) != 2) {
        send_response(fd, 400, "Bad Request", "text/plain", "Bad Request");
        return;
    }

    if (strcmp(method, "GET") && strcmp(method, "POST")) {
        send_response(fd, 405, "Method Not Allowed", "text/plain", "Only GET/POST supported");
        return;
    }

    char path[1024] = {0};
    char* qmark = strchr(target, '?');
    if (qmark) {
        size_t len = (size_t)(qmark - target); if (len >= sizeof(path)) len = sizeof(path) - 1; memcpy(path, target, len); path[len] = '\0';
    } else { snprintf(path, sizeof(path), "%s", target); }
    const char* query = qmark ? (qmark + 1) : NULL;

    long content_length = 0;
    if (strstr(method, "POST")) {
        const char* cl = strcasestr(req, "Content-Length:");
        if (cl) {
            cl += 15; while (*cl == ' ' || *cl == '\t') cl++; content_length = strtol(cl, NULL, 10);
            if (content_length < 0 || content_length > 32768) { content_length = 0; }
        }
        size_t body_have = body_ptr ? (size_t)(r - (body_ptr - req)) : 0;
        while (body_have < (size_t)content_length && body_have < 32768) {
            ssize_t more = recv(fd, req + r, sizeof(req) - 1 - r, 0);
            if (more <= 0) break; r += more; req[r] = '\0'; body_have = body_ptr ? (size_t)(r - (body_ptr - req)) : 0;
        }
    }

    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/health") == 0) {
            handle_health(fd);
        } else if (strcmp(path, "/cities") == 0) {
            handle_cities(fd);
        } else if (strcmp(path, "/weather") == 0) {
            handle_weather(fd, query);
        } else {
            send_response(fd, 404, "Not Found", "application/json", "{\"error\":\"not found\"}");
        }
        return;
    }

    /* POST handlers */
    if (!body_ptr) { send_response(fd, 400, "Bad Request", "application/json", "{\"error\":\"missing body\"}"); return; }
    size_t body_len = (size_t)(r - (body_ptr - req));
    char* body_json = (char*)malloc(body_len + 1); if (!body_json) { send_response(fd, 500, "Internal Server Error", "application/json", "{\"error\":\"oom\"}"); return; }
    memcpy(body_json, body_ptr, body_len); body_json[body_len] = '\0';
    cJSON* root = cJSON_Parse(body_json);
    if (!root) { free(body_json); send_response(fd, 400, "Bad Request", "application/json", "{\"error\":\"invalid json\"}"); return; }

    if (strcmp(path, "/city") == 0) {
        cJSON* nameItem = cJSON_GetObjectItemCaseSensitive(root, "name");
        if (!cJSON_IsString(nameItem) || !nameItem->valuestring) {
            cJSON_Delete(root); free(body_json); send_response(fd, 400, "Bad Request", "application/json", "{\"error\":\"missing name\"}"); return;
        }
        const char* cityName = nameItem->valuestring;
        Cities cities = {0}; if (cities_init(&cities) != 0) { cJSON_Delete(root); free(body_json); send_response(fd, 500, "Internal Server Error", "application/json", "{\"error\":\"cities_init failed\"}"); return; }
        City* found = NULL;
        if (cities_get(&cities, (char*)cityName, &found) == 0) {
            cJSON* resp = cJSON_CreateObject(); cJSON_AddStringToObject(resp, "name", found->name); cJSON_AddNumberToObject(resp, "latitude", found->latitude); cJSON_AddNumberToObject(resp, "longitude", found->longitude); char* out = cJSON_PrintUnformatted(resp); send_response(fd, 200, "OK", "application/json", out); cJSON_free(out); cJSON_Delete(resp); cities_dispose(&cities); cJSON_Delete(root); free(body_json); return;
        }
        if (city_add_from_api((char*)cityName, &cities) != 0 || cities_get(&cities, (char*)cityName, &found) != 0) {
            cities_dispose(&cities); cJSON_Delete(root); free(body_json); send_response(fd, 404, "Not Found", "application/json", "{\"error\":\"city not found\"}"); return;
        }
        cJSON* resp = cJSON_CreateObject(); cJSON_AddStringToObject(resp, "name", found->name); cJSON_AddNumberToObject(resp, "latitude", found->latitude); cJSON_AddNumberToObject(resp, "longitude", found->longitude); char* out = cJSON_PrintUnformatted(resp); send_response(fd, 201, "Created", "application/json", out); cJSON_free(out); cJSON_Delete(resp); cities_dispose(&cities); cJSON_Delete(root); free(body_json); return;
    } else if (strcmp(path, "/weather") == 0) {
        cJSON* cityItem = cJSON_GetObjectItemCaseSensitive(root, "city");
        if (!cJSON_IsString(cityItem) || !cityItem->valuestring) { cJSON_Delete(root); free(body_json); send_response(fd, 400, "Bad Request", "application/json", "{\"error\":\"missing city\"}"); return; }
        char city_param[256]; snprintf(city_param, sizeof(city_param), "%s", cityItem->valuestring); trim_inplace(city_param);
        Cities cities = {0}; if (cities_init(&cities) != 0) { cJSON_Delete(root); free(body_json); send_response(fd, 500, "Internal Server Error", "application/json", "{\"error\":\"cities_init failed\"}"); return; }
        City* found = NULL; if (cities_get(&cities, city_param, &found) != 0) { if (city_add_from_api(city_param, &cities) != 0 || cities_get(&cities, city_param, &found) != 0) { cities_dispose(&cities); cJSON_Delete(root); free(body_json); send_response(fd, 404, "Not Found", "application/json", "{\"error\":\"city not found\"}"); return; } }
        char url[256]; snprintf(url, sizeof(url), "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current_weather=true", (double)found->latitude, (double)found->longitude);
        Meteo* m = NULL; int net_rc = networkhandler_get_data(url, &m, FLAG_WRITE); if (net_rc != 0 || !m || !m->data) { if (m) { free(m->data); free(m);} cities_dispose(&cities); cJSON_Delete(root); free(body_json); send_response(fd, 502, "Bad Gateway", "application/json", "{\"error\":\"failed to fetch weather\"}"); return; }
        send_response(fd, 200, "OK", "application/json; charset=utf-8", m->data); free(m->data); free(m); cities_dispose(&cities); cJSON_Delete(root); free(body_json); return;
    } else {
        cJSON_Delete(root); free(body_json); send_response(fd, 404, "Not Found", "application/json", "{\"error\":\"not found\"}"); return;
    }
}

static void parse_args(int argc, char** argv, ServerConfig* cfg) {
    snprintf(cfg->addr, sizeof(cfg->addr), "%s", "0.0.0.0");
    cfg->port = HTTP_PORT;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            int p = atoi(argv[++i]);
            if (p > 0 && p < 65536) cfg->port = p;
        } else if ((strcmp(argv[i], "--addr") == 0 || strcmp(argv[i], "--bind") == 0) && i + 1 < argc) {
            snprintf(cfg->addr, sizeof(cfg->addr), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [--addr IP] [--port PORT]\n", argv[0]);
            exit(0);
        } else if (argv[i][0] != '\0' && argv[i][0] != '-') {
            /* Positional: treat as port if numeric */
            int p = atoi(argv[i]);
            if (p > 0) cfg->port = p;
        }
    }
    const char* env_port = getenv("SERVER_PORT");
    if (env_port) {
        int p = atoi(env_port); if (p > 0 && p < 65536) cfg->port = p;
    }
    const char* env_addr = getenv("SERVER_ADDR");
    if (env_addr && *env_addr) {
        snprintf(cfg->addr, sizeof(cfg->addr), "%s", env_addr);
    }
}

int main(int argc, char** argv) {
    ServerConfig cfg; parse_args(argc, argv, &cfg);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (strcmp(cfg.addr, "0.0.0.0") == 0 || strcmp(cfg.addr, "*") == 0) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_aton(cfg.addr, &addr.sin_addr) == 0) {
            fprintf(stderr, "Invalid bind address '%s'\n", cfg.addr);
            close(s); return 1;
        }
    }
    addr.sin_port = htons(cfg.port);

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
    printf("HTTP server listening on %s:%d\n", cfg.addr, cfg.port);

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
