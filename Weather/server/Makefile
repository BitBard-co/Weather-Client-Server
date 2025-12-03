CC := gcc
CFLAGS := -Wall -Wextra -g
INCLUDES := -I../includes -I. -I/usr/include/x86_64-linux-gnu

# OpenSSL Homebrew path (harmless on Linux if absent)
OPENSSL_PATH := /opt/homebrew/opt/openssl@3
CFLAGS += -I$(OPENSSL_PATH)/include
LDFLAGS := -L$(OPENSSL_PATH)/lib -lssl -lcrypto

# --------------------
# HTTP server only
# --------------------

HTTP_BIN := server_http
HTTP_SRC := http_server.c \
		 ../projekt/cache.c ../projekt/cities.c ../projekt/city.c \
		 ../projekt/http.c ../projekt/meteo.c ../projekt/networkhandler.c \
		 ../projekt/parsedata.c ../projekt/utils.c \
		 ../src/libs/cJSON/cJSON.c

all: $(HTTP_BIN)

$(HTTP_BIN): $(HTTP_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(HTTP_SRC) -o $(HTTP_BIN) $(LDFLAGS)

# Run from repo root so cache/ and cities/ live next to Weather/
run: all
	cd .. && ./server/$(HTTP_BIN)

clean:
	rm -f server_http *.o http_server.o

.PHONY: all run clean
