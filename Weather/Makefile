# Weather CLI Application Makefile
# For Linux/WSL environment

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c17 -Wno-deprecated-non-prototype
INCLUDES = -I./includes -I./src/libs/cJSON -I/usr/include/x86_64-linux-gnu
LDFLAGS = -lssl -lcrypto -lcurl

# Directories
SRC_DIR = projekt
BUILD_DIR = build
CJSON_DIR = src/libs/cJSON

# Automatically find all source files
PROJECT_SOURCES := $(wildcard $(SRC_DIR)/*.c)
CJSON_SOURCES := $(wildcard $(CJSON_DIR)/*.c)
MAIN_SOURCE = main.c

# Convert to object file paths
OBJECTS := $(PROJECT_SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS += $(CJSON_SOURCES:%.c=$(BUILD_DIR)/%.o)
MAIN_OBJ := $(BUILD_DIR)/main.o

# Target executable
TARGET = Weather

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	mkdir -p $(BUILD_DIR)/$(CJSON_DIR)

$(TARGET): $(BUILD_DIR) $(MAIN_OBJ) $(OBJECTS)
	$(CC) $(MAIN_OBJ) $(OBJECTS) $(LDFLAGS) -o $@

# Compile .c → .o
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run program
run: $(TARGET)
	./$(TARGET)

# Build subprojects (optional)
server:
	$(MAKE) -C server

client:
	$(MAKE) -C client

run-server: server
	cd server && ./server

run-client: client
	cd client && ./client

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	$(MAKE) -C server clean 2>/dev/null || true
	$(MAKE) -C client clean 2>/dev/null || true

clean-all: clean
	rm -rf cache cities

help:
	@echo "=== Weather build system ==="
	@echo " make          - Build everything"
	@echo " make run      - Build and run Weather"
	@echo " make server   - Build TCP server"
	@echo " make client   - Build TCP client"
	@echo " make clean    - Remove build files"
	@echo " make clean-all - Full cleanup including cache"
	@echo "============================="

.PHONY: all run server client run-server run-client clean clean-all help
