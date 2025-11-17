#ifndef __CACHE_H__
#define __CACHE_H__

/* Forward-deklaration – vi behöver bara veta att structen finns */
struct NetworkHandler;

int cache_write_file(char* _Hash, char* _Data, const char* _Path);
int cache_read_file(char* _Filename, struct NetworkHandler** _NhPtr, const char* _Path);
int cache_check_file(char* _Filename, const char* _Path);

#endif
