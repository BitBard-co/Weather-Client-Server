  #include "../includes/networkhandler.h"
  #include "../includes/cache.h"
  #include <stdio.h>
  #include <errno.h>
  #include <stdlib.h>
  #include <string.h>

  /*Skapar fil*/
int cache_write_file(char* _HashedName, char* _Data, const char* _Path) {
    if (!_HashedName || !_Data || !_Path)
        return -1;

    char filepath[50];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", _Path, _HashedName);

    FILE* fptr = fopen(filepath, "wb"); 
    if (!fptr) {
        perror("fopen");
        return -1;
    }

    fprintf(fptr, "%s", _Data);
    fclose(fptr);

    return 0;
}

  /*Läser från fil o skickar datan till vår NH pointer*/
  int cache_read_file(char* _HashedName, NetworkHandler** _NhPtr, const char* _Path) {
    if (!_HashedName || !_NhPtr || !_Path)
        return -1;

    *_NhPtr = NULL;

    char filepath[50];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", _Path, _HashedName);

    FILE* fptr = fopen(filepath, "rb");
    if (!fptr) {
        perror("fopen");
        return -1;
    }

    if (fseek(fptr, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(fptr);
        return -1;
    }

    long size = ftell(fptr);
    if (size < 0) {
        perror("ftell");
        fclose(fptr);
        return -1;
    }

    fseek(fptr, 0, SEEK_SET);

    NetworkHandler* nh = calloc(1, sizeof(*nh));
    if (!nh) {
        perror("calloc");
        fclose(fptr);
        return -1;
    }

    nh->data = malloc(size + 1);
    if (!nh->data) {
        perror("malloc");
        free(nh);
        fclose(fptr);
        return -1;
    }

    size_t bytes_read = fread(nh->data, 1, size, fptr);
    if (bytes_read != (size_t)size) {
        perror("fread");
        free(nh->data);
        free(nh);
        fclose(fptr);
        return -1;
    }

    nh->size = (size_t)size;
    nh->data[nh->size] = '\0';

    fclose(fptr);
    *_NhPtr = nh;
    return 0;
}

  /*Check if file exists*/
int cache_check_file(char* _HashedName, const char* _Path) {
    if (!_HashedName || !_Path)
        return -1;

    char filepath[50];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", _Path, _HashedName);

    FILE* fptr = fopen(filepath, "r");
    if (!fptr)
        return 1;

    fclose(fptr);
    return 0;
}
