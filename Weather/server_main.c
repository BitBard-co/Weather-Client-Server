// Enable POSIX prototypes (e.g., nanosleep)
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "includes/cities.h"
#include "includes/city.h"
#include "includes/networkhandler.h"
#include "Robinsfile/smw.h"
#include <time.h>
#include <unistd.h>
#include "Robinsfile/HTTPServer.h"
#include "Robinsfile/HTTPServerConnection.h"

typedef struct WeatherServerContext {
    Cities cities;
} WeatherServerContext;

static WeatherServerContext* g_server_ctx = NULL;

static int on_request(void* ctx);
static int on_connection(void* _Context, HTTPServerConnection* _Connection);

// Helper: extract query param value from URL string
static int extract_param(const char* url, const char* key, char* out, size_t outsz)
{
    if(!url || !key || !out || outsz == 0) return -1;
    const char* q = strchr(url, '?');
    if(!q) return -1;
    q++;
    size_t keylen = strlen(key);
    while(*q)
    {
        if(strncmp(q, key, keylen) == 0 && q[keylen] == '=')
        {
            const char* val = q + keylen + 1;
            const char* amp = strchr(val, '&');
            size_t len = amp ? (size_t)(amp - val) : strlen(val);
            if(len >= outsz) len = outsz - 1;
            memcpy(out, val, len);
            out[len] = '\0';
            return 0;
        }
        const char* next = strchr(q, '&');
        if(!next) break;
        q = next + 1;
    }
    return -1;
}

static int on_connection(void* _Context, HTTPServerConnection* _Connection)
{
    // Pass the connection itself as the callback context; use global for server context
    (void)_Context; // suppress unused-parameter warning
    HTTPServerConnection_SetCallback(_Connection, _Connection, on_request);
    return 0;
}

static int write_response(TCPClient* tcp, const char* status, const char* ctype, const char* body)
{
    char header[256];
    int blen = body ? (int)strlen(body) : 0;
    int hlen = snprintf(header, sizeof(header),
        "%s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
        status, ctype, blen);
    if(hlen < 0) return -1;
    TCPClient_Write(tcp, (const uint8_t*)header, (size_t)hlen);
    if(blen > 0) TCPClient_Write(tcp, (const uint8_t*)body, (size_t)blen);
    return 0;
}

static int on_request(void* ctx)
{
    HTTPServerConnection* conn = (HTTPServerConnection*)ctx;
    if(conn == NULL || g_server_ctx == NULL) return -1;

    // Only handle GET /weather?city=Name
    if(conn->method == NULL || conn->url == NULL) return -1;

    printf("Server: method=%s url=%s version=%s\n",
           conn->method ? conn->method : "(null)",
           conn->url ? conn->url : "(null)",
           conn->version ? conn->version : "(null)");

    if (strncmp(conn->method, "GET", 3) != 0) {
        write_response(&conn->tcpClient, "HTTP/1.1 405 Method Not Allowed", "text/plain", "");
        TCPClient_Dispose(&conn->tcpClient);
        return 0;
    }

    if (strncmp(conn->url, "/health", 7) == 0) {
        write_response(&conn->tcpClient, "HTTP/1.1 200 OK", "text/plain", "ok");
        TCPClient_Dispose(&conn->tcpClient);
        return 0;
    }

    if (strncmp(conn->url, "/weather", 8) != 0) {
        write_response(&conn->tcpClient, "HTTP/1.1 404 Not Found", "text/plain", "");
        TCPClient_Dispose(&conn->tcpClient);
        return 0;
    }

    char city[128] = {0};
    if (extract_param(conn->url, "city", city, sizeof(city)) != 0) {
        write_response(&conn->tcpClient, "HTTP/1.1 400 Bad Request", "text/plain", "missing city");
        TCPClient_Dispose(&conn->tcpClient);
        return 0;
    }

    printf("Server: requested city=%s\n", city);

    // Lookup city
    City* selected_city = NULL;
    if (cities_get(&g_server_ctx->cities, city, &selected_city) != 0 || selected_city == NULL) {
        write_response(&conn->tcpClient, "HTTP/1.1 404 Not Found", "text/plain", "city not found");
        TCPClient_Dispose(&conn->tcpClient);
        return 0;
    }

    // Build open-meteo URL and fetch (using networkhandler with caching)
    char url[256];
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current_weather=true",
        selected_city->latitude, selected_city->longitude);

    Meteo* meteo = NULL;
    if (networkhandler_get_data(url, &meteo, FLAG_NO_WRITE) != 0 || meteo == NULL || meteo->data == NULL) {
        write_response(&conn->tcpClient, "HTTP/1.1 502 Bad Gateway", "text/plain", "upstream error");
        TCPClient_Dispose(&conn->tcpClient);
        return 0;
    }

    printf("Server: responding with %zu bytes JSON\n", meteo->size);
    write_response(&conn->tcpClient, "HTTP/1.1 200 OK", "application/json", meteo->data);
    TCPClient_Dispose(&conn->tcpClient);
    free(meteo->data);
    free(meteo);
    return 0;
}

int main()
{
    smw_init();
    WeatherServerContext wctx = {0};
    if(cities_init(&wctx.cities) != 0)
    {
        fprintf(stderr, "Failed to initialize cities list\n");
        return EXIT_FAILURE;
    }
    g_server_ctx = &wctx;

    HTTPServer* server = NULL;
    if(HTTPServer_InitiatePtr(on_connection, &wctx, &server) != 0)
    {
        fprintf(stderr, "Failed to start HTTP server\n");
        return EXIT_FAILURE;
    }

    printf("Weather HTTP Server running on port 8080 (GET /weather?city=Name)\n");
    // Drive the simple mini worker task system
    for(;;)
    {
        // Fallback to coarse ms using time(NULL)
        uint64_t ms = (uint64_t)time(NULL) * 1000ULL;
        smw_work(ms);
        // small sleep to avoid busy loop (~1ms)
        struct timespec ts; ts.tv_sec = 0; ts.tv_nsec = 1000000L;
        nanosleep(&ts, NULL);
    }

    HTTPServer_DisposePtr(&server);
    cities_dispose(&wctx.cities);
    return EXIT_SUCCESS;
}
