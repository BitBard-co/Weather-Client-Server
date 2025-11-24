#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../includes/cities.h"
#include "../includes/utils.h"
#include "../includes/meteo.h"
#include "../includes/city.h"
#include "../includes/cache.h"
#include "../includes/parsedata.h"

int city_init(char* _Name, double _Latitude, double _Longitude, City** _CityPtr) {
	if(_Name == NULL || _CityPtr == NULL) {
   return -1; 
  }

	City* _City = (City*)malloc(sizeof(City));
	if(_City == NULL) {
		printf("Failed to allocate memory for new City\n");
		return -1;
	}

	memset(_City, 0, sizeof(City)); /*Nulls all the values in _City struct*/

  /*Sets name, lat and lon for _City struct*/
	_City->name = utils_strdup(_Name);
	if(_City->name == NULL) {
		printf("Failed to allocate memory for City name\n");
		city_dispose(_City);
    return -1;
	}
  _City->latitude = _Latitude;
  _City->longitude = _Longitude;
  _City->next = NULL;
  _City->prev = NULL;


  char filepath[128];
snprintf(filepath, sizeof(filepath), "%s%f%f", _Name, _Latitude, _Longitude);

char* hashed_name = utils_hash_url(filepath);
if (hashed_name == NULL) {
    city_dispose(_City);
    return -1;
}

char* json_str = NULL;

  cJSON* root = cJSON_CreateObject(); /*Creates cJSON root object*/
  if (root == NULL) {
    perror("cJSON_CreateObject");
    free(hashed_name);
    city_dispose(_City);
    return -1;
  }
  
  /*Adds fileds to json root*/
  cJSON_AddStringToObject(root, "name", _Name);
  cJSON_AddNumberToObject(root, "latitude", _Latitude);
  cJSON_AddNumberToObject(root, "longitude", _Longitude);

  /*Creates json string from root object*/
  json_str = cJSON_PrintUnformatted(root);
  if (json_str == NULL) {
    cJSON_Delete(root);
    free(hashed_name);
    city_dispose(_City);
    return -1;
  }

  /*Creates and writes data to cache file in cities/ folder*/
  if (cache_write_file(hashed_name, json_str, CITY_CACHE) != 0) {
    free(hashed_name);
    cJSON_free(json_str);
    cJSON_Delete(root);
    city_dispose(_City);
    return -1;
  }
  
  *(_CityPtr) = _City; /*Needs to be freed by caller*/

  /*Cleanup*/
  free(hashed_name);
  cJSON_Delete(root);
  cJSON_free(json_str);
	
	return 0;
}

int city_get_info(Cities* _Cities) {
    
  char* user_input = NULL;
  if (utils_get_user_input(&user_input) != 0) {
    printf("Please try again\n");
    return -1;
    }

  City* user_city = NULL;

  if (cities_get(_Cities, user_input, &user_city) != 0) { /*Check if City exists*/
    printf("City not found.\n");
    printf("\nWould you like to try to add it from API? Y/N: ");
    if (utils_continue() != 0) { /*Function to capture user input*/
      free(user_input);
      return -1;
    } else {
      
    if (city_add_from_api(user_input, _Cities) != 0) {/*Search for city in API*/
      free(user_input);
      return -1;
    }
    }
    
    if (cities_get(_Cities, user_input, &user_city) != 0) {
      free(user_input);
      return -1;
    }

  }

  meteo_get_weather_data(user_city->latitude, user_city->longitude, user_city->name); /*Request weather data*/
  
/*  free(user_city); */
  free(user_input);
  user_input = NULL;

  cities_dispose(_Cities);
  
  return 0;
}


int city_add_from_api(char* _CityName, Cities* _Cities) {
  if (_CityName == NULL || _Cities == NULL) {
    return -1;
  }

   /*Create copy and replace swedish characters to enable case insensitive search*/
  char name_copy[128];
  snprintf(name_copy, sizeof(name_copy), "%s", _CityName);
  utils_replace_swedish_chars(name_copy);


  cJSON* root = meteo_get_city_data(name_copy); /* first attempt (possibly ASCII-normalized) */
  if (root == NULL) {
    printf("Failed to get city data from API (primary attempt)\n");
    return -1;
  }

  /* If no usable item found, retry with the original (unmodified) spelling including diacritics */
  int need_retry_with_original = 0;
  do {
    double latitude = 0.0; 
    double longitude = 0.0; 
    char name[50];
    cJSON* item = NULL;

    if (cJSON_IsArray(root) && cJSON_GetArraySize(root) > 0) {
        item = cJSON_GetArrayItem(root, 0);
    }
    if (item == NULL && cJSON_IsObject(root)) {
        cJSON *results = cJSON_GetObjectItemCaseSensitive(root, "results"); 
        if (cJSON_IsArray(results) && cJSON_GetArraySize(results) > 0) {
            item = cJSON_GetArrayItem(results, 0);
        }
    }

    if (item == NULL) {
      /* First pass yielded nothing; schedule retry with original diacritic form */
      need_retry_with_original = (need_retry_with_original == 0);
      break;
    }

    snprintf(name, sizeof(name), "%s", parsedata_get_string(item, "name"));
    latitude = parsedata_get_double(item, "latitude");
    longitude = parsedata_get_double(item, "longitude");
    if (latitude == 0.0f && longitude == 0.0f) {
      need_retry_with_original = (need_retry_with_original == 0);
      break;
    }

    if (cities_add(_Cities, name, latitude, longitude, NULL) != 0) {
      cJSON_Delete(root);
      return -1;
    }
    cJSON_Delete(root);
    return 0; /* success added */
  } while (0);

  if (need_retry_with_original) {
    cJSON_Delete(root);
    /* Second attempt with the raw user spelling */
    cJSON* retry_root = meteo_get_city_data(_CityName);
    if (retry_root == NULL) {
      printf("Second attempt failed for '%s'\n", _CityName);
      return -1;
    }
    cJSON* item = NULL;
    if (cJSON_IsArray(retry_root) && cJSON_GetArraySize(retry_root) > 0) {
        item = cJSON_GetArrayItem(retry_root, 0);
    }
    if (item == NULL && cJSON_IsObject(retry_root)) {
        cJSON *results = cJSON_GetObjectItemCaseSensitive(retry_root, "results"); 
        if (cJSON_IsArray(results) && cJSON_GetArraySize(results) > 0) {
            item = cJSON_GetArrayItem(results, 0);
        }
    }
    if (item == NULL) {
      cJSON_Delete(retry_root);
      return -1;
    }
    double latitude = parsedata_get_double(item, "latitude");
    double longitude = parsedata_get_double(item, "longitude");
    char name[50];
    snprintf(name, sizeof(name), "%s", parsedata_get_string(item, "name"));
    if (latitude == 0.0f && longitude == 0.0f) {
      cJSON_Delete(retry_root);
      return -1;
    }
    if (cities_add(_Cities, name, latitude, longitude, NULL) != 0) {
      cJSON_Delete(retry_root);
      return -1;
    }
    cJSON_Delete(retry_root);
    return 0;
  }

  /* If we reached here without retry, original attempt failed in a way we handled */
  return -1;

  /* unreachable legacy block (logic moved above) */
  return -1;

}

void city_dispose(City* _City) {
  if (_City == NULL) {
    return;
  }

  free(_City->name);
  _City->name = NULL;
  free(_City);

}
