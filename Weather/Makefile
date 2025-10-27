# ===============================
# 💻 WeatherCLI Makefile (macOS ARM / Intel)
# ===============================

# Kompilator (clang är standard på macOS)
CC := clang

# Kataloger
SRC_DIR := src
BUILD_DIR := build

# Hämta OpenSSL-prefix via Homebrew
OPENSSL_PREFIX := $(shell brew --prefix openssl@3)

# Kompilatorflaggor
CFLAGS := -std=c17 -Wall -Wextra -Wno-deprecated-non-prototype -Wno-unused-variable -Wno-sign-conversion \
           -Wno-unused-function -g \
           -I$(OPENSSL_PREFIX)/include \
           -Iincludes -Isrc/libs/cjson

# Länkningsflaggor
LDFLAGS := -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto -lcurl

# Käll- och objektfiler
SRC := $(shell find -L $(SRC_DIR) -type f -name '*.c')
OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

# Binärfil
BIN := Weather

# ===============================
# 🧩 Byggmål
# ===============================

# Standardmål
all: $(BIN)
	@echo "✅ Build complete for macOS M3."

# Länkning
$(BIN): $(OBJ)
	@echo "🔗 Linking $@..."
	@$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Bygg varje objektfil
$(BUILD_DIR)/%.o: %.c
	@echo "⚙️  Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Kör programmet
run: $(BIN)
	@echo "🚀 Running WeatherCLI..."
	@./$(BIN)

# ===============================
# 🧹 Hjälpmål
# ===============================

# Rensa
clean:
	@echo "🧹 Cleaning build files..."
	@rm -rf $(BUILD_DIR) $(BIN)

# Debug
print:
	@echo "SRC = $(SRC)"
	@echo "OBJ = $(OBJ)"
	@echo "OPENSSL_PREFIX = $(OPENSSL_PREFIX)"

# ===============================
# 🧠 Extra
# ===============================

# Valgrind (inte tillgänglig på macOS ARM)
valgrind: $(BIN)
	@echo "⚠️  Valgrind not available on macOS ARM64 by default."
	@./$(BIN)

# Ignorera dep-filer om de inte finns ännu
-include $(DEP)

.PHONY: all run clean print valgrind
