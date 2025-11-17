#ifndef __CITY_H__
#define __CITY_H__

/* Forward-deklaration av Cities */
struct Cities;

typedef struct City {
    struct City* prev;
    struct City* next;
    char* name;
    float latitude;
    float longitude;
} City;

int city_get_info(struct Cities* _CityList);
int city_parse_list(struct Cities* _Cities, const char* list);  /* om ni använder den */
int city_init(char* _Name, double _Latitude, double _Longitude, City** _CityPtr);
int city_add_from_api(char* _Name, struct Cities* _Cities);
void city_dispose(City* _City);

#endif
