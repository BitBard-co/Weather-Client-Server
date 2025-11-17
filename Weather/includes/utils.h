#ifndef __UTILS_H__
#define __UTILS_H__

#define WEATHER_DATA_CACHE "cache"
#define CITY_CACHE "cities" 

char* utils_strdup(const char* str); 
int utils_get_user_input(char** _InputPtr);
int utils_break_loop(void);
char* utils_hash_url(char* _URL);
int utils_compare_time(char* _Filename, char* _Path, int _Interval);
int utils_create_folder(const char* _Path);
int utils_continue(void);
int utils_strcasecmp(char* _StringOne, char* _StringTwo);
void utils_replace_swedish_chars(char* _String);

#endif
