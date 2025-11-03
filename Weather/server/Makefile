CC := gcc
CFLAGS := -Wall -Wextra -g
SRC := server.c
OBJ := server.o
BIN := server

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all run clean
