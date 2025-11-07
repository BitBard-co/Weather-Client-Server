#include <stdio.h>
#include <string.h>
#include "../includes/networkhandler.h"
#include "../includes/cache.h"
#include "../includes/utils.h"
#include "../includes/http.h"

/*------------------Internal function definitons------------------*/
void networkhandler_cleanup();

/*---------------------------------------------------------------*/
int networkhandler_get_data(char* _URL, Meteo** _Meteo, int _Flag) {
  if (_Meteo == NULL) {
    return -1;
  }
  
  *(_Meteo) = NULL;

  char* filename = utils_hash_url(_URL); /*create filename hash md5*/
  Meteo* mt = calloc(1, sizeof *mt); /*Allocates nulled meory for Meteo struct*/
 
  if (mt == NULL) {
    fprintf(stderr, "Unable to allocate memory for Meteo\n");
    return -1;
  }

  NetworkHandler* nh = {0}; /*Struct to hold data*/

  if (filename && /*Check if file exists and is not older than specified interval*/
  cache_check_file(filename, WEATHER_DATA_CACHE) == 0 &&
  utils_compare_time(filename, WEATHER_DATA_CACHE, 900) == 0) {
    printf("Reading from file: %s\n", filename);
    if (cache_read_file(filename, &nh, WEATHER_DATA_CACHE) != 0) { /*Read data from file to nh struct*/
      fprintf(stderr, "Failed to read file: %s\n", filename);
      networkhandler_cleanup(nh, filename, mt);
      return -1;
    }
  } else { /*If file does not exist or too old, make api request*/
    printf("Making APIRequest to: %s\n", _URL); 
    if (http_api_request(_URL, &nh) != 0) { /*Populate nh struct with data from api request*/
      fprintf(stderr, "Failed to make api request with url: %s\n", _URL);
      networkhandler_cleanup(nh, filename, mt);
      return -1;
    }
    if (_Flag == 1) { /*Only create file if request is weather data*/
      if (cache_write_file(filename, nh->data, WEATHER_DATA_CACHE) != 0) {
        printf("Failed to create cache file\n");
      }
    }

  }
    
  if (nh->data == NULL || nh->size == 0) {
    fprintf(stderr, "Data is empty\n");
    networkhandler_cleanup(nh, filename, mt);
    return -1;
  }

  mt->data = (char*)malloc(nh->size + 1); /*Size + 1 for null termination*/
  if (mt->data == NULL) {
    fprintf(stderr, "Failed to allocate memory for Meteo data\n");
    networkhandler_cleanup(nh, filename, mt);
    return -1;
  }
  
  memcpy(mt->data, nh->data, nh->size); /*Copy data to meteo struct*/
  mt->size = nh->size;
  mt->data[mt->size] = '\0';
  *(_Meteo) = mt; /*Send data to meteo function, needs to free memory*/

  networkhandler_dispose(nh);
  free(filename);

  return 0;
}

void networkhandler_dispose(NetworkHandler* _Nh){
  if (!_Nh) {
    return;
  }

  free(_Nh->data);
  _Nh->data = NULL;
  _Nh->size = 0;
  free(_Nh);

}

void networkhandler_cleanup(NetworkHandler* _Nh, char* _Filename, Meteo* _Meteo) {
  if (_Meteo != NULL) {
    free(_Meteo->data);
    free(_Meteo);
  }
  networkhandler_dispose(_Nh);
  free(_Filename);
}
