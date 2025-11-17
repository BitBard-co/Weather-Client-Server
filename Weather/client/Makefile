# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-deprecated-non-prototype
INCLUDES = -I../includes -I. -I/usr/include/x86_64-linux-gnu

# Build directories
BUILD_DIR = build
BUILD_PROJ = $(BUILD_DIR)/projekt

# Source files
CLIENT_SRC = weather_client.c
PROJ_SRCS = ../projekt/cache.c ../projekt/cities.c ../projekt/city.c \
            ../projekt/http_server.c ../projekt/http_client.c \
            ../projekt/networkhandler.c ../projekt/parsedata.c \
            ../projekt/utils.c  ../projekt/weather_server.c
CJSON_SRC = cJSON.c

# Homebrew OpenSSL
OPENSSL_PATH := /opt/homebrew/opt/openssl@3
CFLAGS += -I$(OPENSSL_PATH)/include
LDFLAGS += -L$(OPENSSL_PATH)/lib -lssl -lcrypto -lcurl

# Object files
CLIENT_OBJ = $(BUILD_DIR)/client.o
PROJ_OBJS = $(patsubst ../projekt/%.c,$(BUILD_PROJ)/%.o,$(PROJ_SRCS))
CJSON_OBJ = $(BUILD_DIR)/cJSON.o

# Final executable
TARGET = client

all: $(TARGET)

$(CLIENT_OBJ): $(CLIENT_SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_PROJ)/%.o: ../projekt/%.c
	mkdir -p $(BUILD_PROJ)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(CJSON_OBJ): $(CJSON_SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(CLIENT_OBJ) $(PROJ_OBJS) $(CJSON_OBJ)
	$(CC) $^ -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)
