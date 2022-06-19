# Game of Life Simulation

This is an implementation of Conway's Game of Life in C for a university project


## About the Project

This project is command line application written in C using the ncurses library
to display GUI in the terminal. Project works both on Unix and Windows systems.

Features:
- Graphics:
  - Every menu is completely responsive
  - Unicode and No Unicode mode

- Multiple game modes:
  - Normal
  - Coexistence
  - Predator
  - Unknown
  - Virus

- Game:
  - Custom display interval
  - Custom speed
  - Movable cursor
  - Movable screen
  - Optional mouse support in game
  - Status line with information about the game
  - Visual select mode

- Save/Load system:
  - Save/Load complete system
  - Save/Load part of the system

- Variable size system:
  - Enter a custom size for the system, or
  - Play on infinite sized simulation

- Help menu:
  - Information about the game
  - Cheatsheet with common cell patterns

- Documentation:
  - html documentation using doxygen
  - pdf documentation using doxygen and pdflatex


## Prerequisites

- C compiler
- Make
- ncurses (pdcurses on Windows)
- doxygen and pdflatex (if you want to build documentation)


## How to build

Clone the repository with, and enter the directory

```
git clone https://www.github.com/DimitrijeDobrota/GameofLife
cd GameofLife
```

See `Makefile` for the build script.
To get information about all build options run `make help`.

To build the project, simply run `make` at the command line in the current directory.

By default, this will generate an executable called whatever `NAME`
is bound to in `Makefile` (by default, `gol`), using `CC` compiler (by default, `gcc`).
To then run the program, type in

```
./bin/gol
```

in the project directory.

To compile the debugging version of the executable, run `make DEBUG=Y`.
You can also give the optional parameter `NO_UNICODE=Y`, to build the executable without Unicode support.


## Running the project

In the `./bin` directory you will find the executable, and you can run it with

```
./bin/gol
```

If you are on Windows you can double click on `gol.exe` inside `./bin` and
terminal window will pop-up with the game
