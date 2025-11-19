#include "client.h"
#include <curl/curl.h>
#include "../includes/cities.h"
#include "../includes/city.h"
#include "../includes/utils.h"
#include "cJSON.h"

typedef struct {
    char* data;
    size_t size;
} MemBuf;

static size_t write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    MemBuf* mem = (MemBuf*)userp;
    char* ptr = realloc(mem->data, mem->size + total + 1);
    if (!ptr) return 0;
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, total);
    mem->size += total;
    mem->data[mem->size] = '\0';
    return total;
}

static int http_get(const char* url, long* http_code, char** out_body) {
    *http_code = 0;
    *out_body = NULL;
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    MemBuf buf = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        free(buf.data);
        return (res == CURLE_COULDNT_CONNECT) ? -2 : -1;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
    curl_easy_cleanup(curl);
    *out_body = buf.data ? buf.data : strdup("");
    return 0;
}

static int print_cities_from_json(const char* json) {
    cJSON* arr = cJSON_Parse(json);
    if (!arr || !cJSON_IsArray(arr)) {
        cJSON_Delete(arr);
        return -1;
    }
    int idx = 1;
    printf("\nAvailable cities:\n");
    cJSON* it = NULL;
    cJSON_ArrayForEach(it, arr) {
        if (cJSON_IsString(it) && it->valuestring) {
            printf("  %2d) %s\n", idx++, it->valuestring);
        }
    }
    cJSON_Delete(arr);
    return 0;
}

static void list_cities(const char* base_url) {
    char url[512];
    snprintf(url, sizeof(url), "%s/cities", base_url);
    long code = 0; char* body = NULL;
    int rc = http_get(url, &code, &body);
    if (rc == -2) {
        printf("❌ Not connected to HTTP server (%s).\n", base_url);
        free(body);
        return;
    } else if (rc != 0) {
        printf("⚠️  Failed to query cities.\n");
        free(body);
        return;
    }
    if (code != 200) {
        printf("⚠️  /cities returned HTTP %ld\n", code);
        free(body);
        return;
    }
    if (print_cities_from_json(body) != 0) {
        printf("⚠️  Could not parse cities list.\n");
    }
    free(body);
}

static void fetch_weather(const char* base_url, const char* city_name) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("⚠️  curl init failed.\n");
        return;
    }
    char* enc = curl_easy_escape(curl, city_name, 0);
    char url[768];
    snprintf(url, sizeof(url), "%s/weather?city=%s", base_url, enc ? enc : "");
    curl_free(enc);

    long code = 0; char* body = NULL;
    int rc = http_get(url, &code, &body);
    if (rc == -2) {
        printf("❌ Not connected to HTTP server (%s).\n", base_url);
        free(body);
        curl_easy_cleanup(curl);
        return;
    } else if (rc != 0) {
        printf("⚠️  Request failed.\n");
        free(body);
        curl_easy_cleanup(curl);
        return;
    }
    if (code != 200) {
        printf("⚠️  /weather returned HTTP %ld: %s\n", code, body ? body : "");
        free(body);
        curl_easy_cleanup(curl);
        return;
    }

    /* For now, print raw JSON response from server */
    printf("\nWeather JSON for '%s':\n%s\n\n", city_name, body);
    free(body);
    curl_easy_cleanup(curl);
}

void start_client_http(const char* base_url) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    printf("🌤️  Welcome! (HTTP mode)\n");
    printf("Server: %s\n", base_url);

    char buffer[256];
    while (1) {
        list_cities(base_url);
        printf("\nEnter city name (or 'quit' to exit): ");
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "quit") == 0) {
            printf("👋 Goodbye!\n");
            break;
        }
        if (buffer[0] == '\0') continue;
        fetch_weather(base_url, buffer);
    }
    curl_global_cleanup();
}

int main(int argc, char** argv) {
    const char* base = DEFAULT_HTTP_BASE;
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        base = argv[1];
    }
    start_client_http(base);
    return 0;
}