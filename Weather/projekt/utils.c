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

int utils_compare_time(char* _Filename, char* _Path, int _Interval) {
    if (_Filename == NULL || _Path == NULL) {
        return -1;
    }

    char filepath[60];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", _Path, _Filename);

    struct stat file_info;
    if (stat(filepath, &file_info) == -1) {
        perror("stat");
        return -1;
    }

    time_t now = time(NULL);
    double diff = difftime(now, file_info.st_mtime);

    if (diff < _Interval) { 
        return 0;   /* filen är "ny" nog */
    } else { 
        return 1;   /* filen är för gammal */
    }
}

char* utils_hash_url(char* _URL) {
    if (_URL == NULL) {
        return NULL;
    }

    unsigned int i;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Could not create EVP_MD_CTX\n");
        return NULL;
    }

    if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1) {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return NULL;
    }

    if (EVP_DigestUpdate(ctx, _URL, strlen(_URL)) != 1) {
        fprintf(stderr, "EVP_DigestUpdate failed\n");
        EVP_MD_CTX_free(ctx);
        return NULL;
    }

    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        fprintf(stderr, "EVP_DigestFinal_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return NULL;
    }

    EVP_MD_CTX_free(ctx);

    char* md5_string = (char*)malloc(digest_len * 2 + 1);
    if (md5_string == NULL) {
        fprintf(stderr, "malloc failed in utils_hash_url\n");
        return NULL;
    }

    for (i = 0; i < digest_len; i++) {
        snprintf(&md5_string[i * 2], 3, "%02x", digest[i]);
    }
    md5_string[digest_len * 2] = '\0';

    return md5_string; /* Needs to be freed by caller */
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

    /* UTF-8 sequences (two bytes) */
    if(source[0] == 0xC3 && source[1] != '\0') { /* åäöÅÄÖ all start with C3 in utf-8 */
      switch(source[1]) { /* second byte */
        case 0xA5: /* å */
        case 0x85: /* Å */
        case 0xA4: /* ä */
        case 0x84: /* Ä */
          *(destination)++ = 'a';
          source += 2;
          continue;
        case 0xB6: /* ö */
        case 0x96: /* Ö */
          *(destination)++ = 'o';
          source += 2;
          continue;
      }
    }

    /* Latin-1 single byte forms (common in Windows console / CP1252 input) */
    switch(source[0]) {
      case 0xE5: /* å */
      case 0xC5: /* Å */
      case 0xE4: /* ä */
      case 0xC4: /* Ä */
        *(destination)++ = 'a';
        source++;
        continue;
      case 0xF6: /* ö */
      case 0xD6: /* Ö */
        *(destination)++ = 'o';
        source++;
        continue;
    }

    /* Handle combining marks that can appear after base letters (e.g., a + \u0308, A + \u030A) */
    if (source[0] == 0xCC && source[1] != '\0') {
      switch (source[1]) {
        case 0x88: /* \u0308 COMBINING DIAERESIS */
        case 0x8A: /* \u030A COMBINING RING ABOVE */
          source += 2; /* skip mark */
          continue;
      }
    }

    *(destination)++ = *(source)++; /* copy byte */
  }

  *(destination) = '\0';

}


