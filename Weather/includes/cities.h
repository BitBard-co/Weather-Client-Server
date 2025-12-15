#ifndef __CITIES_H__
#define __CITIES_H__

/* Framdclara City så vi kan ha pekare till den */
typedef struct City City;

typedef struct Cities {
    City* head;
    City* tail;
    City* list; 
    int   count;  
} Cities;

extern const char* cities_list;

int cities_get(Cities* _Cities, char* _Name, City** _CityPtr);
int cities_init(Cities* _Cities);
void cities_print(Cities* _Cities);
int cities_add(Cities* _Cities, char* _Name, float _Latitude, float _Longitude, City** _City);
void cities_dispose(Cities* _Cities);
void cities_free(Cities* _Cities);

#endif
