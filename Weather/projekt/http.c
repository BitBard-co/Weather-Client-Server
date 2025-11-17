#include "../includes/networkhandler.h"
#include "../includes/http.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int http_api_request(char* _URL, NetworkHandler** _NhPtr) {
    if (_URL == NULL || _NhPtr == NULL) {
        return -1;
    }
    *_NhPtr = NULL;

    NetworkHandler* nh = calloc(1, sizeof *nh); /*Create nh struct, zero-initialized*/
    if (nh == NULL) {
        fprintf(stderr, "Failed to allocate NetworkHandler\n");
        return -1;
    }

    CURL *handle = curl_easy_init();
    if (handle == NULL) {
        fprintf(stderr, "curl_easy_init failed\n");
        free(nh);
        return -1;
    }

    /* Make API request with CURL */
    curl_easy_setopt(handle, CURLOPT_URL, _URL);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)nh);
  
    CURLcode result = curl_easy_perform(handle);

    if (result != CURLE_OK) {
        fprintf(stderr, "Failed to fetch data from API: %s\n",
                curl_easy_strerror(result));
        curl_easy_cleanup(handle);
        free(nh);
        return -1;
    }

    curl_easy_cleanup(handle);
    *(_NhPtr) = nh; /* Needs to be freed by caller */

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
