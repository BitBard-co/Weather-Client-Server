#include "../src/libs/cJSON/cJSON.h"
#include "../includes/parsedata.h"

double parsedata_get_double(cJSON* _Root, const char* _Name) {
    if (_Root == NULL || _Name == NULL) {
        return 0.0;
    }

    cJSON* field = cJSON_GetObjectItemCaseSensitive(_Root, _Name);
    if (!cJSON_IsNumber(field)) {
        return 0.0;
    }

    return field->valuedouble;
}

const char* parsedata_get_string(cJSON* _Root, const char* _Name) {
    if (_Root == NULL || _Name == NULL) {
        return "Unknown";
    }

    cJSON* field = cJSON_GetObjectItemCaseSensitive(_Root, _Name);
    if (!cJSON_IsString(field)) {
        return "Unknown";
    }

    return field->valuestring;
}

int parsedata_get_int(cJSON* _Root, const char* _Name) {
    if (_Root == NULL || _Name == NULL) {
        return 0;
    }

    cJSON* field = cJSON_GetObjectItemCaseSensitive(_Root, _Name);
    if (!cJSON_IsNumber(field)) {
        return 0;
    }

    return field->valueint;
}
