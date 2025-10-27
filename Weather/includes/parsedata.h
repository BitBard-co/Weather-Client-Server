#ifndef __PARSEDATA_H__
#define __PARSEDATA_H__
#include "../src/libs/cJSON/cJSON.h"

double parsedata_get_double(cJSON* _Root, const char* _Name);
const char* parsedata_get_string(cJSON* _Root, const char* _Name);
int parsedata_get_int(cJSON* _Root, const char* _Name);

#endif 

