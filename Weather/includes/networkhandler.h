#ifndef __NETWORKHANDLER_H__
#define __NETWORKHANDLER_H__

#include <stdlib.h>
#include "../includes/meteo.h"

#define FLAG_WRITE 1
#define FLAG_NO_WRITE 0

typedef struct NetworkHandler {
    char *data;
    size_t size;
    int sockfd;
} NetworkHandler;

int networkhandler_get_data(char* _URL, Meteo** _Meteo, int _Flag);
void networkhandler_dispose(NetworkHandler* _Nh);

#endif
