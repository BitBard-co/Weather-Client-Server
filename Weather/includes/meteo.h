#ifndef __METEO_H__
#define __METEO_H__
#include <stdlib.h>
#include "../includes/parsedata.h"

typedef struct {
  char* data;
  size_t size;
} Meteo;

typedef struct {
    char* name;
    double latitude;
    double longitude;
    double generationtime_ms;
    int utc_offset_seconds;
    char timezone[16];
    char timezone_abbreviation[8];
    double elevation;
    char time[32];
    int interval;
    double temperature;
    double windspeed;
    int winddirection;
    int is_day;
    int weathercode;
} MeteoWeatherData;

int meteo_get_weather_data(double _Latitude, double _Longitude, char* _CityName);
cJSON* meteo_get_city_data(char* _CityName);

#endif
