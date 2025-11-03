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

# Source files (main weather application)
SOURCES = main.c \
          $(SRC_DIR)/cache.c \
          $(SRC_DIR)/cities.c \
          $(SRC_DIR)/city.c \
          $(SRC_DIR)/http.c \
          $(SRC_DIR)/meteo.c \
          $(SRC_DIR)/networkhandler.c \
          $(SRC_DIR)/parsedata.c \
          $(SRC_DIR)/utils.c

# cJSON source files
CJSON_SOURCES = $(CJSON_DIR)/cJSON.c

# Object files  
MAIN_OBJ = $(BUILD_DIR)/main.o
PROJ_OBJECTS = $(filter-out main.c,$(SOURCES))
OBJECTS = $(PROJ_OBJECTS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CJSON_OBJECTS = $(CJSON_SOURCES:$(CJSON_DIR)/%.c=$(BUILD_DIR)/%.o)

# Main executable
TARGET = Weather

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build main executable
$(TARGET): $(BUILD_DIR) $(MAIN_OBJ) $(OBJECTS) $(CJSON_OBJECTS)
	$(CC) $(MAIN_OBJ) $(OBJECTS) $(CJSON_OBJECTS) $(LDFLAGS) -o $@

# Compile main.c
$(MAIN_OBJ): main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile projekt source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile cJSON source files
$(BUILD_DIR)/%.o: $(CJSON_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run the weather application
run: $(TARGET)
	./$(TARGET)

# Build server
server: $(BUILD_DIR)
	$(MAKE) -C server

# Build client
client: $(BUILD_DIR)
	$(MAKE) -C client

# Run server
run-server: server
	cd server && ./server

# Run client  
run-client: client
	cd client && ./client

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	$(MAKE) -C server clean 2>/dev/null || true
	$(MAKE) -C client clean 2>/dev/null || true

# Clean all generated files including caches
clean-all: clean
	rm -rf cache cities

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the main Weather application"
	@echo "  run        - Build and run the Weather application"
	@echo "  server     - Build the TCP server"
	@echo "  client     - Build the TCP client"
	@echo "  run-server - Build and run the TCP server"
	@echo "  run-client - Build and run the TCP client"
	@echo "  clean      - Remove build files"
	@echo "  clean-all  - Remove build files and cache directories"
	@echo "  help       - Show this help message"

.PHONY: all run server client run-server run-client clean clean-all help