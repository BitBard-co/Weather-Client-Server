#include "libs/cJSON/cJSON.h"
#include "../includes/parsedata.h"

double parsedata_get_double(cJSON* _Root, const char* _Name) {
  cJSON* field = cJSON_GetObjectItemCaseSensitive(_Root, _Name);
  double _Fallback = 0.0f;
  if (!cJSON_IsNumber(field)) {
    return _Fallback;
  }
  return field->valuedouble;
}

const char* parsedata_get_string(cJSON* _Root, const char* _Name) {
  cJSON* field = cJSON_GetObjectItemCaseSensitive(_Root, _Name);
  const char* _Fallback = "Unknown";
  if (!cJSON_IsString(field)) {
    return _Fallback;
  }
  return field->valuestring;
}


int parsedata_get_int(cJSON* _Root, const char* _Name) {
  cJSON* field = cJSON_GetObjectItemCaseSensitive(_Root, _Name);
  int _Fallback = 0;
  if (!cJSON_IsNumber(field)) {
    return _Fallback;
  }
  return field->valueint;
}
