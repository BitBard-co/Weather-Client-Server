#include <stdio.h>
#include <string.h>
#include "../includes/cities.h"
#include "../includes/utils.h"
#include "../includes/meteo.h"
#include "../includes/city.h"
#include "../includes/cache.h"
#include "../includes/parsedata.h"

/*-------------Internal function definitions----------------*/


/*--------------------------------------------------------*/


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


  char* json_str;
  char* hashed_name;
  char filepath[50];
  sprintf(filepath, "%s%f%f", _Name, _Latitude, _Longitude); /*creates base for filename citylatlon*/
  hashed_name = utils_hash_url(filepath); /*Hashed filename with md5 hash*/


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
    free(user_input);
    printf("Please try again\n");
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
    strcpy(name_copy, _CityName);
    name_copy[sizeof(name_copy) - 1] = '\0';
    utils_replace_swedish_chars(name_copy);

  cJSON* root = NULL; /*Create cJSON root object*/
  root = meteo_get_city_data(name_copy); /*Get city data from Meteo*/
  if (root == NULL) {
    printf("Failed to get city data from API\n");
    cJSON_Delete(root);
    return -1;
  }

  double latitude, longitude = 0.0f; /*Initialize name, lat, lon*/
  char name[50];
  cJSON* item = NULL;

  if (cJSON_IsArray(root) && cJSON_GetArraySize(root) > 0) { /*Check if root is an array and larger than 0*/
      item = cJSON_GetArrayItem(root, 0); /*Set item to first "item" in root array*/
  }

  if (item == NULL && cJSON_IsObject(root)) { /*If root is an object not array*/
      cJSON *results = cJSON_GetObjectItemCaseSensitive(root, "results"); 
      if (cJSON_IsArray(results)) {
          item = cJSON_GetArrayItem(results, 0); /*Set item to first item in results array*/
      }
  }

  strcpy(name, parsedata_get_string(item, "name"));
  latitude = parsedata_get_double(item, "latitude");
  longitude = parsedata_get_double(item, "longitude");
  if (latitude == 0.0f && longitude == 0.0f) {
    printf("No city data available\n\n");
    cJSON_Delete(root);
    return -1;
  }

  if (cities_add(_Cities, name, latitude, longitude, NULL) != 0) { /*Adds city to Cities list*/
    cJSON_Delete(root);
    return -1;
  }

  cJSON_Delete(root); /*Free root*/

  return 0;

}

void city_dispose(City* _City) {
  if (_City == NULL) {
    return;
  }

  free(_City->name);
  _City->name = NULL;
  free(_City);

}
