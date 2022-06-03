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
int isExpanding;

char *evolution_names[] = {"Normal", "CoExsistance", "Predator", "Virus",
                           "Unknown"};
int   evolution_cells[] = {2, 3, 3, 3, 3};
int   evolution_size = 5;
int   toggle_mod = 2;

static void (*evolve)(void);
static void (*addToCells)(int i, int j, int value);

void insert(int row, int col, int value) {
  temp_cells[temp_size].val = value;
  temp_cells[temp_size].row = row;
  temp_cells[temp_size].col = col;
  temp_size++;
}

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
        insert(k, l, value);
        continue;
      }
      insert(k, l, mod);
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

  for (int k = i - 1; k <= i + 1; k++) {
    for (int l = j - 1; l <= j + 1; l++) {
      int a = (k + HEIGHT) % HEIGHT;
      int b = (l + WIDTH) % WIDTH;
      for (int m = 0; m < temp_size; m++)
        if (temp_cells[m].row == a && temp_cells[m].col == b) {
          temp_cells[m].val += mod;
          if (a == i && b == j) {
            temp_cells[m].val -= mod;
            temp_cells[m].val += value;
          }
          goto Kontinue;
        }

      if (a == i && b == j) {
        insert(a, b, value);
        continue;
      }
      insert(a, b, mod);
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

void do_evolution(int steps) {
  while (steps--) {
    int temp_alloc = 9 * cells_size;
    temp_size = 0;
    temp_cells = calloc(temp_alloc, sizeof(*temp_cells));
    evolve();
    free(temp_cells);
  }
}

int logic_init(int w, int h, int isWrapping) {
  WIDTH = w;
  HEIGHT = h;

  addToCells = addition_modes[isWrapping];

  cells = malloc(WIDTH * HEIGHT * sizeof(*cells));
  cells_size = 0;
  return 1;
}

int evolution_init(int index) {
  evolve = evolution_modes[index];
  toggle_mod = evolution_cells[index];
  return 1;
}

int logic_free(void) {
  free(cells);
  cells = NULL;

  HEIGHT = -1;
  WIDTH = -1;
  addToCells = NULL;
  evolve = NULL;
  toggle_mod = -1;
  return 1;
}

int toggleAt(int i, int j) {
  for (int k = 0; k < cells_size; k++) {
    if (cells[k].row == i && cells[k].col == j) {
      cells[k].val = (cells[k].val + 1) % toggle_mod;
      if (cells[k].val == 0) {
        for (int t = k + 1; t < cells_size; t++) {
          cells[t - 1].val = cells[t].val;
          cells[t - 1].row = cells[t].row;
          cells[t - 1].col = cells[t].col;
        }
        cells_size--;
        return 0; // since the cell was deleted it's value is 0
      }
      return cells[k].val; // only return value if it hasn't been deleted
    }
  }
  cells[cells_size].val = 1;
  cells[cells_size].row = i;
  cells[cells_size].col = j;
  cells_size++;
  return 1;
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

int getAt(int i, int j) {
  for (int k = 0; k < cells_size; k++)
    if (cells[k].row == i && cells[k].col == j)
      return cells[k].val;
  return 0;
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
  return 1;
}
