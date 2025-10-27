# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -g -Wno-deprecated-non-prototype \
          -I/opt/homebrew/opt/openssl@3/include \
          -I../includes \
          -Isrc/libs/cjson

# Directories
PROJ_DIR := ../projekt
BUILD_DIR := build
CJSON_DIR := src/libs/cjson

# Source files
PROJ_SRC := cache.c cities.c city.c http.c meteo.c networkhandler.c parsedata.c utils.c
PROJ_OBJ := $(addprefix $(BUILD_DIR)/projekt/, $(PROJ_SRC:.c=.o))

CJSON_SRC := $(CJSON_DIR)/cJSON.c $(CJSON_DIR)/cJSON_Utils.c
CJSON_OBJ := $(addprefix $(BUILD_DIR)/cjson/, $(notdir $(CJSON_SRC:.c=.o)))

CLIENT_SRC := client.c
CLIENT_OBJ := $(BUILD_DIR)/client.o

# Binary
BIN := client

# Default target
all: $(BIN)

# Link all objects
$(BIN): $(CLIENT_OBJ) $(PROJ_OBJ) $(CJSON_OBJ)
	$(CC) $^ -o $@ -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto -lcurl

# Compile client
$(BUILD_DIR)/client.o: client.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile project source files
$(BUILD_DIR)/projekt/%.o: $(PROJ_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile cJSON source files
$(BUILD_DIR)/cjson/%.o: $(CJSON_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Run the binary
run: $(BIN)
	./$(BIN)

# Clean build
clean:
	rm -rf $(BUILD_DIR) $(BIN)

.PHONY: all run clean
