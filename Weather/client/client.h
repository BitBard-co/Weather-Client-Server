#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_HTTP_BASE "http://127.0.0.1:8080"

/* Run interactive HTTP client against server_http.
 * base_url example: "http://127.0.0.1:8080" */
void start_client_http(const char* base_url);

#endif // CLIENT_H
