CFLAGS = -I include
LDFLAGS = -lncursesw

SRC = src
OBJ = obj
BINDIR = bin

BIN = bin/gol
SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

CFLAGS +=-ggdb -Wall

all: ${BIN}

$(BIN): $(OBJS)
		${CC} $^ $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
		${CC} -c $< -o $@ $(CFLAGS)

clean:
		rm -r $(BINDIR)/* $(OBJ)/*

.PHONY: all clean
