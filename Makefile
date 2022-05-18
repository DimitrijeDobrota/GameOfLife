CC=gcc
CFLAGS = -I include

ifeq ($(OS),Windows_NT)
LDFLAGS = -lpdcurses
else
LDFLAGS = -lncurses
endif

SRC = src
OBJ = obj
BINDIR = bin

BIN = bin/gol
SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

all: $(BIN)
debug: CFLAGS +=-ggdb -Wall
debug: ${BIN}

$(BIN): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

ifeq ($(OS),Windows_NT)
clean:
	del bin\gol.exe $(subst /,\,$(OBJS))
else
clean:
	rm -f $(BIN) $(OBJS)
endif

.PHONY: all clean debug
