#include "../includes/cache.h"
#include "../includes/networkhandler.h"
#include <stdio.h>
#include <errno.h>

/*Skapar fil*/
int cache_write_file(char* _HashedName, char* _Data, const char* _Path) {
  if (_HashedName == NULL || _Data == NULL || _Path == NULL) {
    return -1;
  }  

  char filepath[50];
  sprintf(filepath, "%s/%s.json", _Path, _HashedName);
  FILE* fptr;
  fptr = fopen(filepath, "w");
  if (fptr == NULL) {
    perror("fopen");
    fclose(fptr);
    return -1;
  }

  fprintf(fptr, "%s", _Data);
  fclose(fptr);

  return 0; 
}

/*Läser från fil o skickar datan till vår NH pointer*/
int cache_read_file(char* _HashedName, NetworkHandler** _NhPtr, const char* _Path) {
  if (_HashedName == NULL || _NhPtr == NULL || _Path == NULL) {
    return -1;
  }

  *_NhPtr = NULL;
  FILE* fptr;
  char filepath[50];
  sprintf(filepath, "%s/%s.json",_Path, _HashedName);
  
  fptr = fopen(filepath, "rb");
  if (fptr == NULL) {
    perror("fopen");
    return -1;
  }

  if (fseek(fptr, 0, SEEK_END) != 0) { /*Counts filesize in bytes*/
    perror("fseek");
    fclose(fptr);
    return -1;
  }

  long size = ftell(fptr); /*Writes filesize to size variable*/
  if (size < 0) {
    perror("ftell");
    fclose(fptr);
    return -1;
  }

  if (fseek(fptr, 0, SEEK_SET) != 0) { /*Resets filepoint to start*/
    perror("fseek");
    fclose(fptr);
    return -1;
  }

  NetworkHandler* nh = calloc(1, sizeof(*nh)); /*Allocates initiliazed memory for nh struct*/
  if (nh == NULL) {
    perror("calloc");
    fclose(fptr);
    return -1;
  }

  nh->data = (char*)malloc((size_t)size + 1); /*Allocates size + 1 for data field*/
  if (nh->data == NULL) {
    perror("malloc");
    fclose(fptr);
    free(nh);
    return -1;
  }
  
  size_t bytes_read = fread(nh->data, 1, size, fptr); /*Checks whole file was read*/
  if (bytes_read != (size_t)size) {
    perror("fread");
    fclose(fptr);
    free(nh->data);
    free(nh);
    return -1;
  }

  nh->size = (size_t)size;
  nh->data[nh->size] = '\0';

  fclose(fptr);

  *(_NhPtr)= nh; /*Sets input ptr to new pointer, needs to be freed by caller*/

  return 0;
}   

/*Check if file exists*/
int cache_check_file(char* _HashedName, const char* _Path) {
  if (_HashedName == NULL || _Path == NULL) {
    return -1;
  }

  char filepath[50];
  sprintf(filepath, "%s/%s.json",_Path, _HashedName);
  
  FILE* fptr;
  fptr = fopen(filepath, "r");
  if (fptr == NULL) {
    return 1; /*File does not exist*/
  }

  fclose(fptr);
  return 0; /*File exists*/
}
