# GNU Makefile for Game of Life simulation
#
# Usage: make [-f path\Makefile] [DEBUG=Y] [NO_UNICODE=Y] target

NAME = gol
CC = gcc

CFLAGS = -I include

SRC = src
OBJ = obj
BINDIR = bin
LATEX = docs/latex

BIN = bin/$(NAME)
SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

ifeq ($(OS),Windows_NT)
	LDFLAGS = -lpdcurses
	RM = del
	NAME := $(NAME).exe
	DEL_CLEAN = $(subst /,\,$(BIN)) $(subst /,\,$(OBJS))
else
	LDFLAGS = -lncurses
	RM = rm -f
	DEL_CLEAN = $(BIN) $(OBJS)
endif

ifeq ($(DEBUG),Y)
	CFLAGS += -ggdb -Wall
endif

ifeq ($(NO_UNICODE),Y)
	CFLAGS += -D NO_UNICODE
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	-$(RM) $(DEL_CLEAN)

docs:
	doxygen
	make -C $(LATEX)

help:
	@echo "Game of Life Simulation"
	@echo
	@echo "Usage: make [-f path\Makefile] [DEBUG=Y] [NO_UNICODE=Y] target"
	@echo
	@echo "Target rules:"
	@echo "    all         - Compiles binary file [Default]"
	@echo "    clean       - Clean the project by removing binaries"
	@echo "    help        - Prints a help message with target rules"
	@echo "    docs        - Compile html and pdf documentation using doxygen and pdflatex"
	@echo
	@echo "Optional parameters:"
	@echo "    DEBUG       - Compile binary file with debug flags enabled"
	@echo "    NO_UNICODE  - Compile binary file that does not use Unicode characters"
	@echo

.PHONY: all clean help docs
