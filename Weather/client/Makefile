# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-deprecated-non-prototype
INCLUDES = -I../includes -I. -I/usr/include/x86_64-linux-gnu
LDFLAGS = -lssl -lcrypto -lcurl

# Build directories
BUILD_DIR = build
BUILD_PROJ = $(BUILD_DIR)/projekt

# Source files
CLIENT_SRC = client.c
PROJ_SRCS = ../projekt/cache.c ../projekt/cities.c ../projekt/city.c ../projekt/http.c ../projekt/meteo.c ../projekt/networkhandler.c ../projekt/parsedata.c ../projekt/utils.c
CJSON_SRC = cJSON.c

# 🔹 Lägg till Homebrew OpenSSL-sökväg
OPENSSL_PATH := /opt/homebrew/opt/openssl@3
CFLAGS += -I$(OPENSSL_PATH)/include
LDFLAGS += -L$(OPENSSL_PATH)/lib -lssl -lcrypto -lcurl

# Object files
CLIENT_OBJ = $(BUILD_DIR)/client.o
PROJ_OBJS = $(patsubst ../projekt/%.c,$(BUILD_PROJ)/%.o,$(PROJ_SRCS))
CJSON_OBJ = $(BUILD_DIR)/cJSON.o

# Final executable
TARGET = client

# Default target
all: $(TARGET)

# Build client object
$(CLIENT_OBJ): $(CLIENT_SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build project objects
$(BUILD_PROJ)/%.o: ../projekt/%.c
	mkdir -p $(BUILD_PROJ)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build cJSON object
$(CJSON_OBJ): $(CJSON_SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link all objects
$(TARGET): $(CLIENT_OBJ) $(PROJ_OBJS) $(CJSON_OBJ)
	$(CC) $^ -o $(TARGET) $(LDFLAGS)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)
