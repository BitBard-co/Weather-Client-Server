#include "../includes/utils.h"
#include <string.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

int utils_continue() {
  int c, d; 
  c = getchar();              /* Read first char as response*/
  if (c == '\r') c = getchar(); /* On some environments '\r' is added, if thats the case jump to next char */
    /* Empty the rest of the line for future inputs */
  while ((d = getchar()) != '\n' && d != EOF) { }

    /*If input is n/N exit to main loop*/
  if (c == 'n' || c == 'N') {
    return -1;
  }
  return 0;
}


char* utils_strdup(const char* str) {
  if (!str) return NULL;
  size_t len = strlen(str) + 1;
  char* copy = (char*)malloc(len);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, str, len);   
  
  return copy; /*Caller needs to free*/
}

int utils_get_user_input(char** _InputPtr) {
  int buff_size = 25;
  char* user_input = (char*)malloc(buff_size);
  if (user_input == NULL) {
    printf("Unable to allocate memory for user input\n");
    return -1;
  }
  printf("Input city: ");
  if (fgets(user_input, buff_size, stdin) == NULL) {
    free(user_input);
    return -1;
  }

  size_t len = strlen(user_input);
  if (len > 0 && user_input[len-1] == '\n') {
    user_input[len-1] = '\0';   /* newline fick plats, ta bort den */
  } else {
    /* newline låg kvar i stdin, töm resten av raden */
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

  if ((_InputPtr) != NULL) {
    *(_InputPtr) = user_input;
  }

  return 0;
}

int utils_break_loop() {
  printf("Would you like to get the weather for another city? Y/N: ");
  char ch;

  while ((ch = getchar()) != '\n' && ch != EOF) { 
     if (ch == 'N' || ch == 'n') {
    return 1;
    }
  }
   return 0;
}

int utils_create_folder(const char* _Path) {
	#if defined _WIN32
		bool success = CreateDirectory(_Path, NULL);
		if(success == false)
		{
			DWORD err = GetLastError();
			if(err == ERROR_ALREADY_EXISTS)
				return 1;
			else
				return -1;

		}
	#else
		int success = mkdir(_Path, 0777);
		if(success != 0)
		{
			if(errno == EEXIST)
				return 1;
			else
				return -1;

		}
	#endif

	return 0;
}

int utils_compare_time(char* _Filename, char* _Path, int _Interval){
  char filepath[60];
  sprintf(filepath, "%s/%s.json",_Path, _Filename);

	struct stat file_info;
    if (stat(filepath, &file_info) == -1) {
        perror("stat");
        return -1;
    }

    time_t now = time(NULL);
    double diff = difftime(now, file_info.st_mtime);

    if (diff < _Interval) { 
      return 0;
    } else { 
      return 1;
    }
}

char* utils_hash_url(char* _URL){
  unsigned int i;

  unsigned char digest[EVP_MAX_MD_SIZE]; /* Allocate buffer for all functions */
  unsigned int digest_len; /* Actual length in bytes of final hash*/

  EVP_MD_CTX *ctx = EVP_MD_CTX_new(); /*OBject that saves state, hashing algoritm etc*/
  if (!ctx) {
      fprintf(stderr, "Could not create EVP_MD_CTX\n");
      return NULL;
  }

  if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1) { /*Sets MD5 as algoritm for ctx object*/
      fprintf(stderr, "EVP_DigestInit_ex failed\n");
      EVP_MD_CTX_free(ctx);
      return NULL;
  }

  if (EVP_DigestUpdate(ctx, _URL, strlen(_URL)) != 1) { /*Inputs the bytes from text to ctx object*/
      fprintf(stderr, "EVP_DigestUpdate failed\n");
      EVP_MD_CTX_free(ctx);
      return NULL;
  }

  if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) { /*Finalizes data transfer and fills digest buffer with bytes (16for md5)*/
      fprintf(stderr, "EVP_DigestFinal_ex failed\n");
      EVP_MD_CTX_free(ctx);
      return NULL;
  }

  EVP_MD_CTX_free(ctx); /*Free ctx object so it doesnt leak*/

  char* md5_string = (char*)malloc(digest_len * 2 + 1);
   /*cReate buffer for hashed string*/
  
  for (i = 0; i < digest_len; i++) { /*Convert to hex*/
      sprintf(&md5_string[i*2], "%02x", digest[i]);
  }
  md5_string[digest_len * 2] = '\0';

  
  return md5_string; /* Needs to be freed by caller*/
}

int utils_strcasecmp(char* _StringOne, char* _StringTwo) {
    while (*_StringOne && *_StringTwo) {
        unsigned char ch1 = (unsigned char)*_StringOne++;
        unsigned char ch2 = (unsigned char)*_StringTwo++;
        if (tolower(ch1) != tolower(ch2)) {
            return -1;
        }
    }
    return *_StringOne - *_StringTwo;
}

void utils_replace_swedish_chars(char* _String) {
  if (_String == NULL) {
    return;
  }

  unsigned char* source = (unsigned char*)_String;
  unsigned char* destination = (unsigned char*)_String;

  while(*source) {

    /*Check utf-8 sequences, first byte = source[0]*/
    if(source[0] == 0xC3 && source[1] != '\0') { /* åäöÅÄÖ all start with C3 in utf-8 */
      switch(source[1]) { /*check second byte*/
        case 0xA5: /*å*/
        case 0x85: /*Å*/
        case 0xA4: /*ä*/
        case 0x84: /*Ä*/
          *(destination)++ = 'a';
          (source) += 2; /*If found we jump 2 bytes to the next char*/
          continue;
        case 0xB6: /*ö*/
        case 0x96: /*Ö*/
          *(destination)++ = 'o';
          (source) += 2; /*If found we jump 2 bytes to the next char*/
          continue;
      }
    }

    /* Handle combining marks that can appear after base letters (e.g., a + \u0308, A + \u030A) */
    if (source[0] == 0xCC && source[1] != '\0') {
      switch (source[1]) {
        case 0x88: /* \u0308 COMBINING DIAERESIS */
        case 0x8A: /* \u030A COMBINING RING ABOVE */
          /* Skip combining mark entirely to normalize to base letter */
          source += 2;
          continue;
      }
    }

    *(destination)++ = *(source)++; /*Copy current byte
    multibyte chars will be copied over multiple iterations*/
  }

  *(destination) = '\0';

}


