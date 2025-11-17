#ifndef WEATHER_SERVER_H
#define WEATHER_SERVER_H

#include "meteo.h"   // must contain Meteo + MeteoWeatherData definitions

// Core API
char* weather_server_get_json(double lat, double lon, const char *name);

// Optional utilities
void meteo_print_weatherdata(MeteoWeatherData* data);
void meteo_print_full_weatherdata(MeteoWeatherData* data);
void meteo_dispose(Meteo* m);

#endif // WEATHER_SERVER_H