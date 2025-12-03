#include "../includes/networkhandler.h"
#include "../includes/http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct {
    char scheme[8];
    char host[256];
    char port[8];
    char path[1024];
    int use_tls;
} url_parts;

static int parse_url(const char* url, url_parts* out) {
    if (!url || !out) return -1;
    memset(out, 0, sizeof(*out));

    const char* p = strstr(url, "://");
    if (!p) return -1;
    size_t sch_len = (size_t)(p - url);
    if (sch_len >= sizeof(out->scheme)) return -1;
    memcpy(out->scheme, url, sch_len);
    out->scheme[sch_len] = '\0';
    for (size_t i = 0; i < sch_len; ++i) out->scheme[i] = (char)tolower(out->scheme[i]);
    out->use_tls = (strcmp(out->scheme, "https") == 0);

    const char* host_start = p + 3;
    const char* path_start = strchr(host_start, '/');
    const char* host_end = path_start ? path_start : url + strlen(url);

    const char* colon = memchr(host_start, ':', (size_t)(host_end - host_start));
    size_t host_len;
    if (colon) {
        host_len = (size_t)(colon - host_start);
        size_t port_len = (size_t)(host_end - colon - 1);
        if (port_len >= sizeof(out->port)) return -1;
        memcpy(out->port, colon + 1, port_len);
        out->port[port_len] = '\0';
    } else {
        host_len = (size_t)(host_end - host_start);
        snprintf(out->port, sizeof(out->port), "%s", out->use_tls ? "443" : "80");
    }
    if (host_len == 0 || host_len >= sizeof(out->host)) return -1;
    memcpy(out->host, host_start, host_len);
    out->host[host_len] = '\0';

    if (path_start) {
        size_t path_len = strlen(path_start);
        if (path_len >= sizeof(out->path)) return -1;
        memcpy(out->path, path_start, path_len);
        out->path[path_len] = '\0';
    } else {
        strcpy(out->path, "/");
    }
    return 0;
}

static int connect_tcp(const char* host, const char* port) {
    struct addrinfo hints; memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = NULL;
    int rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(rc));
        return -1;
    }
    int sock = -1;
    for (struct addrinfo* it = res; it; it = it->ai_next) {
        sock = (int)socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (sock < 0) continue;
        if (connect(sock, it->ai_addr, (socklen_t)it->ai_addrlen) == 0) {
            break;
        }
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);
    if (sock < 0) {
        fprintf(stderr, "connect failed\n");
        return -1;
    }
    return sock;
}

static int ssl_read_all(SSL* ssl, NetworkHandler* nh) {
    char buf[4096];
    for (;;) {
        int n = SSL_read(ssl, buf, (int)sizeof buf);
        if (n > 0) {
            size_t newsize = nh->size + (size_t)n;
            char* newptr = realloc(nh->data, newsize + 1);
            if (!newptr) return -1;
            nh->data = newptr;
            memcpy(nh->data + nh->size, buf, (size_t)n);
            nh->size = newsize;
            nh->data[nh->size] = '\0';
        } else {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_ZERO_RETURN) break; /* clean shutdown */
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
            break;
        }
    }
    return 0;
}

static int tcp_read_all(int sock, NetworkHandler* nh) {
    char buf[4096];
    for (;;) {
        ssize_t n = recv(sock, buf, sizeof buf, 0);
        if (n > 0) {
            size_t newsize = nh->size + (size_t)n;
            char* newptr = realloc(nh->data, newsize + 1);
            if (!newptr) return -1;
            nh->data = newptr;
            memcpy(nh->data + nh->size, buf, (size_t)n);
            nh->size = newsize;
            nh->data[nh->size] = '\0';
        } else if (n == 0) {
            break;
        } else {
            if (errno == EINTR) continue;
            return -1;
        }
    }
    return 0;
}

int http_api_request(char* _URL, NetworkHandler** _NhPtr) {
    if (_URL == NULL || _NhPtr == NULL) {
        return -1;
    }
    *_NhPtr = NULL;

    url_parts parts;
    if (parse_url(_URL, &parts) != 0) {
        fprintf(stderr, "Invalid URL: %s\n", _URL);
        return -1;
    }

    NetworkHandler* nh = calloc(1, sizeof *nh);
    if (!nh) {
        fprintf(stderr, "Failed to allocate NetworkHandler\n");
        return -1;
    }

    int sock = connect_tcp(parts.host, parts.port);
    if (sock < 0) { free(nh); return -1; }

    char req[2048];
    int req_len = snprintf(req, sizeof req,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: WeatherServer/1.0\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n\r\n",
        parts.path, parts.host);
    if (req_len <= 0 || (size_t)req_len >= sizeof req) {
        fprintf(stderr, "Request build failed\n");
        close(sock);
        free(nh);
        return -1;
    }

    int rc = -1;
    if (parts.use_tls) {
        SSL_library_init();
        SSL_load_error_strings();
        const SSL_METHOD* method = TLS_client_method();
        SSL_CTX* ctx = SSL_CTX_new(method);
        if (!ctx) { close(sock); free(nh); return -1; }

        SSL* ssl = SSL_new(ctx);
        if (!ssl) { SSL_CTX_free(ctx); close(sock); free(nh); return -1; }
        SSL_set_tlsext_host_name(ssl, parts.host);
        SSL_set_fd(ssl, sock);
        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl); SSL_CTX_free(ctx); close(sock); free(nh); return -1;
        }

        int wn = SSL_write(ssl, req, req_len);
        if (wn <= 0) { SSL_free(ssl); SSL_CTX_free(ctx); close(sock); free(nh); return -1; }

        rc = ssl_read_all(ssl, nh);

        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
    } else {
        ssize_t wn = send(sock, req, (size_t)req_len, 0);
        if (wn < 0) { close(sock); free(nh); return -1; }
        rc = tcp_read_all(sock, nh);
    }

    close(sock);
    if (rc != 0) { free(nh); return -1; }

    *(_NhPtr) = nh;
    return 0;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    size_t bytes = size * nmemb;
    NetworkHandler *nh = (NetworkHandler *)userp;
    if (nh == NULL || buffer == NULL || bytes == 0) {
        return 0;
    }
    size_t newsize = nh->size + bytes;
    char *newptr = realloc(nh->data, newsize + 1);
    if (newptr == NULL) {
        printf("Unable to reallocate memory\n");
        return 0;
    }
    nh->data = newptr;
    memcpy(nh->data + nh->size, buffer, bytes);
    nh->size = newsize;
    nh->data[nh->size] = '\0';
    return bytes;
}
