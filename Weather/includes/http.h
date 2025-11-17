#ifndef __HTTP_H__
#define __HTTP_H__

#include <stdlib.h>
#include "../includes/networkhandler.h"

int http_api_request(char* _URL, NetworkHandler** _NhPtr);
size_t write_data(void* _Data, size_t _Size, size_t _Element_count, void* _Userp);

#endif
