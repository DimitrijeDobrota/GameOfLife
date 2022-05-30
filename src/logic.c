#include <stdio.h>
#include <stdlib.h>

#include "logic.h"
#include "utils.h"

#define SIZE_TO_EXPAND 10

typedef struct Cell {
  unsigned char val;
  int           row;
  int           col;
} Cell;

// GLOBALS
Cell *cells;
Cell *temp_cells;

int WIDTH, HEIGHT;
int cells_size;
int temp_size;
int counter;

char *evolution_names[] = {"Normal", "CoExsistance", "Predator", "Virus",
                           "Unknown"};
int   evolution_cells[] = {2, 3, 3, 3, 3};
int   evolution_size = 5;
static void (*evolve)(void);
static void (*addToCells)(int i, int j, int value);

void addToCellsNormal(int i, int j, int value) {
  int mod = 0;
  switch (value) {
  case 1:
    mod = 4;
    break;
  case 2:
    mod = 32;
    break;
  }
  for (int k = i - 1; k <= i + 1; k++) {
    for (int l = j - 1; l <= j + 1; l++) {
      for (int m = 0; m < temp_size; m++)
        if (temp_cells[m].row == k && temp_cells[m].col == l) {
          temp_cells[m].val += mod;
          if (k == i && l == j) {
            temp_cells[m].val -= mod;
            temp_cells[m].val += value;
          }
          goto Kontinue;
        }

      if (k == i && l == j) {
        temp_cells[temp_size].val = value;
        temp_cells[temp_size].row = k;
        temp_cells[temp_size].col = l;
        temp_size++;
        continue;
      }
      temp_cells[temp_size].val = mod;
      temp_cells[temp_size].row = k;
      temp_cells[temp_size].col = l;
      temp_size++;
    Kontinue:
      continue;
    }
  }
}

void addToCellsWrap(int i, int j, int value) {
  int mod = 0;
  switch (value) {
  case 1:
    mod = 4;
    break;
  case 2:
    mod = 32;
    break;
  }

  for (int k = (i - 1 + HEIGHT) % HEIGHT; k <= (i + 1 + HEIGHT) % HEIGHT; k++) {
    for (int l = (j - 1 + WIDTH) % WIDTH; l <= (j + 1 + WIDTH) % WIDTH; l++) {
      for (int m = 0; m < temp_size; m++)
        if (temp_cells[m].row == k && temp_cells[m].col == l) {
          temp_cells[m].val += mod;
          if (k == i && l == j) {
            temp_cells[m].val -= mod;
            temp_cells[m].val += value;
          }
          goto Kontinue;
        }

      if (k == i && l == j) {
        temp_cells[temp_size].val = value;
        temp_cells[temp_size].row = k;
        temp_cells[temp_size].col = l;
        temp_size++;
        continue;
      }
      temp_cells[temp_size].val = mod;
      temp_cells[temp_size].row = k;
      temp_cells[temp_size].col = l;
      temp_size++;
    Kontinue:
      continue;
    }
  }
}

void doAdditions(void) {
  for (int cellIndex = 0; cellIndex < cells_size; cellIndex++) {
    addToCells(cells[cellIndex].row, cells[cellIndex].col,
               cells[cellIndex].val);
  }
}

/* Evolutions */
void evolveNormal(void) {
  doAdditions();
  counter = 0;
  for (int i = 0; i < temp_size; i++) {
    switch (temp_cells[i].val) {
    case 9:
    case 12:
    case 13:
      cells[counter].val = 1;
      cells[counter].row = temp_cells[i].row;
      cells[counter].col = temp_cells[i].col;
      counter++;
      break;
    }
  }
  cells_size = counter;
}

void evolveCoExist(void) {
  doAdditions();
  counter = 0;
  int s1, s2, mod;
  for (int i = 0; i < temp_size; i++) {
    s2 = temp_cells[i].val >> 5;
    s1 = (temp_cells[i].val & 31) >> 2;
    mod = temp_cells[i].val & 3;
    if (mod == 0) {
      if ((s1 + s2) == 3) {
        if (temp_cells[i].val >= 64)
          cells[counter].val = 2;
        else
          cells[counter].val = 1;
        cells[counter].row = temp_cells[i].row;
        cells[counter].col = temp_cells[i].col;
        counter++;
        continue;
      }
    }
    if ((s1 + s2) < 2 || (s1 + s2) > 3) {
      continue;
    }
    cells[counter].val = mod;
    cells[counter].row = temp_cells[i].row;
    cells[counter].col = temp_cells[i].col;
    counter++;
  }
  cells_size = counter;
}

void evolvePredator(void) {
  doAdditions();
  counter = 0;
  int s1, s2, mod;
  for (int i = 0; i < temp_size; i++) {
    s2 = temp_cells[i].val >> 5;
    s1 = (temp_cells[i].val & 31) >> 2;
    mod = temp_cells[i].val & 3;
    if ((s1 + s2) < 2 || (s1 + s2) > 3) {
      continue;
    }
    switch (mod) {
    case 0:
      if ((s1 + s2) == 3) {
        if (temp_cells[i].val >= 64)
          cells[counter].val = 2;
        else
          cells[counter].val = 1;
        cells[counter].row = temp_cells[i].row;
        cells[counter].col = temp_cells[i].col;
        counter++;
        continue;
      }
      break;
    case 1:
      if (s2 > 0) {
        continue;
      }
      break;
    }
    cells[counter].val = mod;
    cells[counter].row = temp_cells[i].row;
    cells[counter].col = temp_cells[i].col;
    counter++;
  }
  cells_size = counter;
}

void evolveVirus(void) {
  doAdditions();
  counter = 0;
  int s1, s2, mod;
  for (int i = 0; i < temp_size; i++) {
    s2 = temp_cells[i].val >> 5;
    s1 = (temp_cells[i].val & 31) >> 2;
    mod = temp_cells[i].val & 3;
    if ((s1 + s2) < 2 || (s1 + s2) > 3) {
      continue;
    }
    switch (mod) {
    case 0:
      if ((s1 + s2) == 3) {
        if (temp_cells[i].val >= 64)
          cells[counter].val = 2;
        else
          cells[counter].val = 1;
        cells[counter].row = temp_cells[i].row;
        cells[counter].col = temp_cells[i].col;
        counter++;
        continue;
      }
      break;
    case 1:
      if (s2 > 0) {
        cells[counter].val = 2;
        cells[counter].row = temp_cells[i].row;
        cells[counter].col = temp_cells[i].col;
        counter++;
        continue;
      }
      break;
    }
    cells[counter].val = mod;
    cells[counter].row = temp_cells[i].row;
    cells[counter].col = temp_cells[i].col;
    counter++;
  }
  cells_size = counter;
}

void evolveUnknown(void) { // Assumption 3 ones and 3 twos result in 50/50
                           // chanse of 0 becoming one of them:
  doAdditions();
  counter = 0;
  int s1, s2, mod;
  for (int i = 0; i < temp_size; i++) {
    s2 = temp_cells[i].val >> 5;
    s1 = (temp_cells[i].val & 31) >> 2;
    mod = temp_cells[i].val & 3;
    switch (mod) {
    case 0:
      if (s1 == 3 && s2 == 3) {
        cells[counter].val = rand() % 2 + 1;
        cells[counter].row = temp_cells[i].row;
        cells[counter].col = temp_cells[i].col;
        counter++;
        continue;
      }
      if (s1 == 3) {
        cells[counter].val = 1;
        cells[counter].row = temp_cells[i].row;
        cells[counter].col = temp_cells[i].col;
        counter++;
        continue;
      }
      if (s2 == 3) {
        cells[counter].val = 2;
        cells[counter].row = temp_cells[i].row;
        cells[counter].col = temp_cells[i].col;
        counter++;
        continue;
      }
      break;
    case 1:
      if (s1 < 2 || s1 > 3) {
        continue;
      }
      break;
    case 2:
      if (s2 < 2 || s2 > 3) {
        continue;
      }
      break;
    }
    cells[counter].val = mod;
    cells[counter].row = temp_cells[i].row;
    cells[counter].col = temp_cells[i].col;
    counter++;
  }
  cells_size = counter;
}

/* Initializing functions */
static void (*evolution_modes[])() = {
    evolveNormal, evolveCoExist, evolvePredator, evolveVirus, evolveUnknown};
static void (*addition_modes[])(int i, int j, int value) = {addToCellsNormal,
                                                            addToCellsWrap};

int do_evolution(int steps) {
  int times_resized = 0;
  counter = 0;
  while (steps--) {
    int temp_alloc = 9 * cells_size;
    temp_size = 0;
    temp_cells = calloc(temp_alloc, sizeof(*temp_cells));
    if (!counter) {
      if (shouldExpand()) {
        expand_matrix(SIZE_TO_EXPAND);
        times_resized++;
        counter = SIZE_TO_EXPAND;
      } else {
        counter = 1;
      }
    }
    evolve();
    free(temp_cells);
    counter--;
  }
  counter = 0;
  return times_resized * SIZE_TO_EXPAND;
}

int logic_init(int w, int h, int isWrapping) {
  WIDTH = w;
  HEIGHT = h;
  addToCells = addition_modes[0];
  if (isWrapping) {
    addToCells = addition_modes[1];
  }

  cells = malloc(WIDTH * HEIGHT * sizeof(*cells));
  return 1;
}

int evolution_init(int index) {
  evolve = evolution_modes[index];
  return 1;
}

int shouldExpand(void) {
  for (int i = 0; i < cells_size; i++) {
    if (cells[i].row == 0 || cells[i].row == HEIGHT - 1 || cells[i].col == 0 ||
        cells[i].col == WIDTH - 1) {
      return 1;
    }
  }
  return 0;
}

void expand_matrix(int size) {
  WIDTH += 2 * size;
  HEIGHT += 2 * size;
  cells = realloc(cells, WIDTH * HEIGHT * sizeof(*cells));
  for (int cellIndex = 0; cellIndex < cells_size; cellIndex++) {
    cells[cellIndex].row += size;
    cells[cellIndex].col += size;
  }
}

int logic_free(void) {
  free(cells);
  return 1;
}

void toggleAt(int i, int j) {
  for (int k = 0; k < cells_size; k++) {
    if (cells[k].row == i && cells[k].col == j) {
      cells[k].val = (cells[k].val + 1) % 3;
      if (cells[k].val == 0) {
        for (int t = k + 1; t < cells_size; t++) {
          cells[t - 1].val = cells[t].val;
          cells[t - 1].row = cells[t].row;
          cells[t - 1].col = cells[t].col;
        }
        cells_size--;
      }
      return;
    }
  }
  cells[cells_size].val = 1;
  cells[cells_size].row = i;
  cells[cells_size].col = j;
  cells_size++;
}

void deleteAt(int i, int j) {
  for (int k = 0; k < cells_size; k++) {
    if (cells[k].row == i && cells[k].col == j) {
      for (int t = k + 1; t < cells_size; t++) {
        cells[t - 1].val = cells[t].val;
        cells[t - 1].row = cells[t].row;
        cells[t - 1].col = cells[t].col;
      }
      cells_size--;
      return;
    }
  }
}

int getNext(int *row, int *col, int *value, int reset) {
  static int index = 0;
  if (reset) {
    index = 0;
    return 1;
  }

  if (index == cells_size)
    return 0;

  *row = cells[index].row;
  *col = cells[index].col;
  *value = cells[index].val;
  index++;
}