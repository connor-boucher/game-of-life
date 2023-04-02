SRC = main.c
BIN = game-of-life

CC     = gcc
CFLAGS = -std=gnu99 -Wall -O3

all: options game-of-life

options:
	@echo game-of-life build options:
	@echo "CC     = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"

game-of-life:
	$(CC) -o $(BIN) $(SRC) $(CFLAGS)

clean:
	rm $(BIN)

.PHONY: all options game-of-life clean
