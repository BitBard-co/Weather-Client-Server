#include "../includes/networkhandler.h"
#include "../includes/http.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>



int http_api_request(char* _URL, NetworkHandler** _NhPtr) {
  NetworkHandler* nh = calloc(1, sizeof *nh); /*Creathe nh struct null initialized*/
  /*Make api request with CURL*/
  CURL *handle = curl_easy_init(); 
  curl_easy_setopt(handle, CURLOPT_URL, _URL);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data); /*Callback function*/
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)nh);
  CURLcode result = curl_easy_perform(handle);

  if (result != CURLE_OK) {
    printf("Failed to fetch data from API\n");
    curl_easy_cleanup(handle);
    return -1;
  }

  curl_easy_cleanup(handle);
  *(_NhPtr) = nh; /*Needs to be freed by caller*/

  return 0;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  /*Writes recieved data to struct*/
  size_t bytes = size * nmemb;
  NetworkHandler *nh = (NetworkHandler *)userp;

  size_t newsize = nh->size + bytes;

  char *newptr = realloc(nh->data, newsize + 1); /*Increase memory allocation*/
  if (newptr == NULL) {
    printf("Unable to reallocate memory\n");
    return -1;
  }
  /*Copy data to nh struct*/
  nh->data = newptr;
  memcpy(nh->data, buffer, bytes); 

  nh->size = newsize;
  nh->data[nh->size] = '\0';
  
  return bytes;
}
