#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../includes/utils.h"
#include "../includes/cities.h"
#include "../includes/city.h"
#include "../includes/tinydir.h"
#include "../includes/cache.h"
#include "../includes/networkhandler.h"
#include "../includes/parsedata.h"

/*--------Internal function definitions-------*/
void cities_print(Cities* _cities);
int cities_add_from_string(Cities* _Cities, const char* list);
void cities_add_from_files(Cities* _Cities);
void cities_dispose(Cities* _Cities);
void cities_free(Cities* _Cities);
/*------------------------------------------*/

const char* cities_list = "Stockholm:59.3293:18.0686\n"
  "Göteborg:57.7089:11.9746\n"
  "Malmö:55.6050:13.0038\n"
  "Uppsala:59.8586:17.6389\n"
  "Västerås:59.6099:16.5448\n"
  "Örebro:59.2741:15.2066\n"
  "Linköping:58.4109:15.6216\n"
  "Helsingborg:56.0465:12.6945\n"
  "Jönköping:57.7815:14.1562\n"
  "Norrköping:58.5877:16.1924\n"
  "Lund:55.7047:13.1910\n"
  "Gävle:60.6749:17.1413\n"
  "Sundsvall:62.3908:17.3069\n"
  "Umeå:63.8258:20.2630\n"
  "Luleå:65.5848:22.1567\n"
  "Kiruna:67.8558:20.2253\n";


/*--------------------------------------------*/
int cities_init(Cities* _Cities) {

  memset(_Cities, 0, sizeof(*_Cities)); /*Resets _Cities and sets all values to 0*/

  utils_create_folder(WEATHER_DATA_CACHE); /*Creates cache/ folder for weather data*/
  utils_create_folder(CITY_CACHE); /*Creates cities/ folder for city data*/
  
  cities_add_from_files(_Cities); /*Reads files in cities/ and populates _Cities list*/

  if (cities_add_from_string(_Cities, cities_list) != 0) {
    cities_dispose(_Cities);
    return -1;
  }
  
  return 0;
}

int cities_add_from_string(Cities* _Cities, const char* list) {
  char* list_copy = utils_strdup(list);
  if (list_copy == NULL) {
    printf("Failed to allocate memory for list\n");
    return -1;
  }
/*Break out each city with name, lat and lon from base string*/
  char* ptr = list_copy;
  char* name = NULL;
  char* lat_str = NULL;
  char* lon_str = NULL;
 
  	do {
		if(name == NULL) {
			name = ptr;
		}
		else if(lat_str == NULL) {
			if(*(ptr) == ':') {
				lat_str = ptr + 1;
				*(ptr) = '\0';
			}
		}
		else if(lon_str == NULL) {
			if(*(ptr) == ':') {
				lon_str = ptr + 1;
				*(ptr) = '\0';
			}
		}
		else {
			if(*(ptr) == '\n') {
				*(ptr) = '\0';

				cities_add(_Cities, name, atof(lat_str), atof(lon_str), NULL);

				name = NULL;
				lat_str = NULL;
				lon_str = NULL;
			}
		}

		ptr++;

	} while (*(ptr) != '\0');

  free(list_copy);
    return 0;
}

void cities_print(Cities* _Cities) {
  if (_Cities->head == NULL) {
    printf("List is empty, no cities to print\n");
  } else {
    City* current = _Cities->head;
    
    printf("\n");
    do {
      printf("- %s\n", current->name);
      current = current->next;
    } while (current != NULL);
    printf("\n");
  }
}
/*Adds city to Linked list*/
int cities_add(Cities* _Cities, char* _Name, float _Latitude, float _Longitude, City** _City) {
  if(_Cities == NULL || _Name == NULL) {
    return -1;
  }
 
  City* existing = NULL;
  if (cities_get(_Cities, _Name, &existing) == 0) {
    if (_City != NULL) {
      *(_City) = existing;
    }
    return 1;
  }

	int result = 0;
	City* new_city = NULL;
	
	result = city_init(_Name, _Latitude, _Longitude, &new_city);

  if(result != 0) {
		printf("Failed to initialize City struct! Errorcode: %i\n", result);
    city_dispose(new_city);
		return -1;
	}
    
  if (_Cities->tail == NULL) {
    _Cities->head = new_city;
    _Cities->tail = new_city;
  } else {
    new_city->prev = _Cities->tail;
    _Cities->tail->next = new_city;
    _Cities->tail = new_city;
  }

  if (_City != NULL) {
    *(_City) = new_city;
  }

  return 0;
}

/* Unused */
void cities_remove(Cities* _Cities, City* _City) {
  
	if (_City->next == NULL && _City->prev == NULL) {
		_Cities->tail = NULL;
        _Cities->head = NULL;
    } else if (_City == _Cities->tail) {
		_Cities->tail = _City->prev;
        _City->prev->next = NULL;
    } else if (_City == _Cities->head){
		_Cities->head = _City->next;
        _City->next->prev = NULL;
    } else {
        _City->next->prev = _City->prev;
        _City->prev->next = _City->next;
    }
    
  city_dispose(_City);

  return;
}

int cities_get(Cities* _Cities, char* _Name, City** _CityPtr) {
    if (_Cities == NULL || _Name == NULL) {
        return -1;
    }

    City* current = _Cities->head;

    char name_copy[128];
    snprintf(name_copy, sizeof(name_copy), "%s", _Name);
    utils_replace_swedish_chars(name_copy);

    while (current != NULL) {
        char city_name_copy[128];
        snprintf(city_name_copy, sizeof(city_name_copy), "%s", current->name);
        utils_replace_swedish_chars(city_name_copy);

        if (current->name && utils_strcasecmp(city_name_copy, name_copy) == 0) {
            if (_CityPtr != NULL) {
                *_CityPtr = current;
            }
            return 0;
        }
        current = current->next;
    }

    return -1;
}

void cities_add_from_files(Cities* _Cities) {
  if (_Cities == NULL) {
      return;
  }

  tinydir_dir dir;
  tinydir_open(&dir, CITY_CACHE);

  while (dir.has_next) {
    NetworkHandler* nh = NULL;
    tinydir_file file;
    tinydir_readfile(&dir, &file); /*Reads each file in cities*/

    if (!file.is_dir) { /*If file is not a directory*/
    char filename[50];
    snprintf(filename, sizeof(filename), "%s", file.name);

    size_t len = strlen(filename);
    if (len > 5 && strcmp(filename + len - 5, ".json") == 0) {
      filename[len - 5] = '\0'; // ta bort .json
    }

      
      if (cache_read_file(filename, &nh, CITY_CACHE) != 0) {
        networkhandler_dispose(nh);
        tinydir_close(&dir);
        return;
      }

      cJSON* root = cJSON_Parse(nh->data); 
      if (root == NULL) {
        const char* error_pointer = cJSON_GetErrorPtr();
        if (error_pointer != NULL){
          fprintf(stderr,"JSON error %s\n", error_pointer);
        }

        networkhandler_dispose(nh);
        tinydir_close(&dir);
        return;
      }

      networkhandler_dispose(nh);

      char name[50];
      snprintf(name, sizeof(name), "%s", parsedata_get_string(root, "name"));
      double lat = parsedata_get_double(root, "latitude");
      double lon = parsedata_get_double(root, "longitude");
      if (cities_add(_Cities, name, lat, lon, NULL) != 0) {
        cJSON_Delete(root);
        tinydir_close(&dir);
        return;
      }
      
      cJSON_Delete(root);
    }

    tinydir_next(&dir);
  }

  tinydir_close(&dir);
}

void cities_dispose(Cities* _Cities) {
  if (_Cities == NULL) {
    return;
  }

  City* current = _Cities->head;

  while (current != NULL) {
    City* next = current->next;
    city_dispose(current);
    current = next;
  }
  
  _Cities->head = NULL;
  _Cities->tail = NULL;

}

void cities_free(Cities* _Cities) {
    if (_Cities->list != NULL) {
        free(_Cities->list);
        _Cities->list = NULL;
        _Cities->count = 0;
    }
}
