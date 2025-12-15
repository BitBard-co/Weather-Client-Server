# Weather CLI Application Makefile
# For Linux/WSL environment

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c17 -Wno-deprecated-non-prototype
INCLUDES = -I./includes -I./src/libs/cJSON -I/usr/include/x86_64-linux-gnu
LDFLAGS = -lssl -lcrypto

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

# Target executables
TARGET = Weather
SERVER_TARGET = WeatherServer

all: $(TARGET) $(SERVER_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	mkdir -p $(BUILD_DIR)/$(CJSON_DIR)
	mkdir -p $(BUILD_DIR)/Robinsfile

$(TARGET): $(BUILD_DIR) $(MAIN_OBJ) $(OBJECTS)
	$(CC) $(MAIN_OBJ) $(OBJECTS) $(LDFLAGS) -o $@

# Compile .c → .o
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Server build
SERVER_SOURCE = server_main.c
SERVER_OBJ := $(BUILD_DIR)/server_main.o
ROBINS_SOURCES := Robinsfile/HTTPServer.c Robinsfile/HTTPServerConnection.c Robinsfile/TCPServer.c Robinsfile/TCPClient.c Robinsfile/smw.c
ROBINS_OBJECTS := $(ROBINS_SOURCES:%.c=$(BUILD_DIR)/%.o)

$(SERVER_TARGET): $(BUILD_DIR) $(SERVER_OBJ) $(OBJECTS) $(ROBINS_OBJECTS)
	$(CC) $(SERVER_OBJ) $(OBJECTS) $(ROBINS_OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/Robinsfile/%.o: Robinsfile/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/server_main.o: server_main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run program
run: $(TARGET)
	./$(TARGET)

run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

# Build subprojects (optional)
server:
	$(MAKE) -C server

run-server: server
	cd server && ./server

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(SERVER_TARGET)
 

clean-all: clean
	rm -rf cache cities

help:
	@echo "=== Weather build system ==="
	@echo " make          - Build everything"
	@echo " make run      - Build and run Weather"
	@echo " make server   - Build TCP demo server"
	@echo " make clean    - Remove build files"
	@echo " make clean-all - Full cleanup including cache"
	@echo "============================="

.PHONY: all run server run-server clean clean-all help
