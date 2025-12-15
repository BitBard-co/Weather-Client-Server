#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../includes/networkhandler.h"
#include "../includes/cache.h"
#include "../includes/utils.h"
#include "../includes/http.h"

/*------------------Internal function definitons------------------*/
void networkhandler_cleanup(NetworkHandler* _Nh, char* _Filename, Meteo* _Meteo);

/*---------------------------------------------------------------*/
int networkhandler_get_data(char* _URL, Meteo** _Meteo, int _Flag) {
    if (_URL == NULL || _Meteo == NULL) {
        return -1;
    }
    *(_Meteo) = NULL;

    char* filename = utils_hash_url(_URL); /*create filename hash md5*/
    if (filename == NULL) {
        fprintf(stderr, "Failed to hash URL\n");
        return -1;
    }

    Meteo* mt = calloc(1, sizeof *mt); /*Allocates nulled memory for Meteo struct*/
    if (mt == NULL) {
        fprintf(stderr, "Unable to allocate memory for Meteo\n");
        free(filename);
        return -1;
    }

    NetworkHandler* nh = NULL; /*Struct pointer to hold data*/

    if (cache_check_file(filename, WEATHER_DATA_CACHE) == 0 &&
        utils_compare_time(filename, WEATHER_DATA_CACHE, 900) == 0) {
        /* Check if file exists and is fresh enough */
        printf("Reading from file: %s\n", filename);
        if (cache_read_file(filename, &nh, WEATHER_DATA_CACHE) != 0) { /*Read data from file to nh struct*/
            fprintf(stderr, "Failed to read file: %s\n", filename);
            networkhandler_cleanup(nh, filename, mt);
            return -1;
        }
    } else {
        /*If file does not exist or is too old, make api request*/
        printf("Making APIRequest to: %s\n", _URL); 
        if (http_api_request(_URL, &nh) != 0) { /*Populate nh struct with data from api request*/
            fprintf(stderr, "Failed to make api request with url: %s\n", _URL);
            networkhandler_cleanup(nh, filename, mt);
            return -1;
        }
        if (_Flag == FLAG_WRITE) { /*Only create file if request is weather data*/
            if (cache_write_file(filename, nh->data, WEATHER_DATA_CACHE) != 0) {
                printf("Failed to create cache file\n");
            }
        }
    }
    
    if (nh == NULL || nh->data == NULL || nh->size == 0) {
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
    if (_Nh == NULL) {
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
        _Meteo->data = NULL;
        _Meteo->size = 0;
        free(_Meteo);
    }
    networkhandler_dispose(_Nh);
    free(_Filename); /* free(NULL) is safe */
}
