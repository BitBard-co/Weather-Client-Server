#ifndef __NETWORKHANDLER_H__
#define __NETWORKHANDLER_H__
#include "../includes/meteo.h"
#include <stdlib.h>
#define FLAG_WRITE 1
#define FLAG_NO_WRITE 0


typedef struct {
  char *data;
  size_t size;
  int sockfd;
} NetworkHandler;

int networkhandler_get_data(char* _URL, Meteo** _Meteo, int _Flag);
void networkhandler_dispose();

#endif

