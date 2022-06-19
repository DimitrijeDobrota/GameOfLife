#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "logic.h"
#include "utils.h"

Cell *hash = NULL;

/**
 * @brief function that delets cell from hash table utilazing uthash.h
 */
void deleter(Cell *c) {
  HASH_DEL(hash, c);
  free(c);
}

/**
 * @brief function that returns pointer to the cell in hash table at given
 * position.
 */
Cell *get(int row, int col) {
  Cell *c, t;

  memset(&t, 0, sizeof(Cell));
  t.cord.row = row;
  t.cord.col = col;
  HASH_FIND(hh, hash, &t.cord, sizeof(Cell_cord), c);

  return c;
}

void insert(int row, int col, int val, int mod) {
  Cell *c;

  c = get(row, col);
  if (c == NULL) {
    MEM_CHECK(c = malloc(sizeof(Cell)));
    c->cord.row = row;
    c->cord.col = col;
    c->val = val;
    HASH_ADD(hh, hash, cord, sizeof(Cell_cord), c);
  }
  c->val += mod;
}

extern int width, height;
int        isExpanding;

Cell **save_cells;
int    save_cells_s;
int    save_cells_sm;

int pos_y;
int pos_x;

char *evolution_names[] = {"Normal", "CoExsistance", "Predator", "Virus",
                           "Unknown"};
int   evolution_cells[] = {2, 3, 3, 3, 3};
int   evolution_size = 5;
int   evolve_index;
int   toggle_mod = 2;

static void (*evolve)(void);
static void (*addToCells)(int i, int j, int value);

/**
 * @brief function that adds its value to neighbouring cells;
 */

void addToCellsNormal(int i, int j, int value) {
  int mod = 0;
  switch (value & 3) {
  case 1:
    mod = 4;
    break;
  case 2:
    mod = 32;
    break;
  }
  for (int k = i - 1; k <= i + 1; k++)
    for (int l = j - 1; l <= j + 1; l++)
      if (k != i || l != j)
        insert(k, l, 0, mod);
}

/**
 * @brief simmilar to addToCellsNormal() with exception that it check for
 * corners in case of wrapping;
 */
void addToCellsWrap(int i, int j, int value) {
  int mod = 0;
  switch (value & 3) {
  case 1:
    mod = 4;
    break;
  case 2:
    mod = 32;
    break;
  }

  for (int k = i - 1; k <= i + 1; k++)
    for (int l = j - 1; l <= j + 1; l++) {
      int a = WCLAMP(k, height);
      int b = WCLAMP(l, width);
      if (a != i || b != j)
        insert(a, b, 0, mod);
    }
}

void doAdditions(void) {
  Cell *c;

  Cell *buff[10000];
  int   size = 0;
  for (c = hash; c != NULL; c = c->hh.next)
    buff[size++] = c;

  for (int i = 0; i < size; i++)
    addToCells(buff[i]->cord.row, buff[i]->cord.col, buff[i]->val);
}

/**
 * @brief function responsible for calculation for a game mod called "Normal";
 */
void evolveNormal(void) {
  Cell *c, *c_next;

  doAdditions();
  for (c = hash; c != NULL; c = c_next) {
    c_next = c->hh.next;
    switch (c->val) {
    case 9:
    case 12:
    case 13:
      c->val = 1;
      break;
    default:
      deleter(c);
    }
  }
}

/**
 * @brief function responsible for calculation for a game mod called "CoExist";
 */
void evolveCoExist(void) {
  Cell *c, *c_next;

  doAdditions();
  int s1, s2, mod;
  for (c = hash; c != NULL; c = c_next) {
    c_next = c->hh.next;
    s2 = c->val >> 5;
    s1 = (c->val & 31) >> 2;
    mod = c->val & 3;
    if (mod == 0) {
      if ((s1 + s2) == 3) {
        if (c->val >= 64)
          c->val = 2;
        else
          c->val = 1;
        continue;
      }
    }
    if ((s1 + s2) < 2 || (s1 + s2) > 3) {
      deleter(c);
    }
    c->val = mod;
  }
}

/**
 * @brief function responsible for calculation for a game mod called "Predator";
 */
void evolvePredator(void) {
  Cell *c, *c_next;
  doAdditions();
  int s1, s2, mod;
  for (c = hash; c != NULL; c = c_next) {
    c_next = c->hh.next;
    s2 = c->val >> 5;
    s1 = (c->val & 31) >> 2;
    mod = c->val & 3;
    if ((s1 + s2) < 2 || (s1 + s2) > 3) {
      deleter(c);
      continue;
    }
    switch (mod) {
    case 0:
      if ((s1 + s2) == 3) {
        if (c->val >= 64)
          c->val = 2;
        else
          c->val = 1;
        continue;
      }
      deleter(c);
      break;
    case 1:
      if (s2 > 0) {
        deleter(c);
        continue;
      }
      break;
    }
    c->val = mod;
  }
}

/**
 * @brief function responsible for calculation for a game mod called "Virus";
 */
void evolveVirus(void) {
  Cell *c, *c_next;
  doAdditions();
  int s1, s2, mod;
  for (c = hash; c != NULL; c = c_next) {
    c_next = c->hh.next;
    s2 = c->val >> 5;
    s1 = (c->val & 31) >> 2;
    mod = c->val & 3;
    if ((s1 + s2) < 2 || (s1 + s2) > 3) {
      deleter(c);
      continue;
    }
    switch (mod) {
    case 0:
      if ((s1 + s2) == 3) {
        if (c->val >= 64)
          c->val = 2;
        else
          c->val = 1;
        continue;
      }
      deleter(c);
      break;
    case 1:
      if (s2 > 0) {
        c->val = 2;
        continue;
      }
      break;
    }
    c->val = mod;
  }
}

/**
 * @brief function responsible for calculation for a game mod called "Unknown";
 */
void evolveUnknown(void) { // Assumption 3 ones and 3 twos result in 50/50
                           // chanse of 0 becoming one of them:
  Cell *c, *c_next;
  doAdditions();
  int s1, s2, mod;
  for (c = hash; c != NULL; c = c_next) {
    c_next = c->hh.next;
    s2 = c->val >> 5;
    s1 = (c->val & 31) >> 2;
    mod = c->val & 3;
    switch (mod) {
    case 0:
      if (s1 == 3 && s2 == 3) {
        c->val = rand() % 2 + 1;
        continue;
      }
      if (s1 == 3) {
        c->val = 1;
        continue;
      }
      if (s2 == 3) {
        c->val = 2;
        continue;
      }
      deleter(c);
      break;
    case 1:
      if (s1 < 2 || s1 > 3) {
        deleter(c);
        continue;
      }
      break;
    case 2:
      if (s2 < 2 || s2 > 3) {
        deleter(c);
        continue;
      }
      break;
    }
    c->val = mod;
  }
}

/* Initializing functions */
static void (*evolution_modes[])() = {
    evolveNormal, evolveCoExist, evolvePredator, evolveVirus, evolveUnknown};
static void (*addition_modes[])(int i, int j, int value) = {addToCellsNormal,
                                                            addToCellsWrap};

/**
 * @brief parent function that calls evolution;
 */
void do_evolution(int steps) {
  while (steps--) {
    evolve();
  }
}

/**
 * @brief init function for game logic;
 */
int logic_init(int isWrapping, int index) {
  save_cells_s = 0;
  save_cells_sm = 100;
  MEM_CHECK(save_cells = malloc(save_cells_sm * sizeof(struct Cell *)));

  addToCells = addition_modes[isWrapping];
  evolve = evolution_modes[index];
  evolve_index = index;
  toggle_mod = evolution_cells[index];
  return 1;
}

/**
 * @brief memory cleaner for logic.c;
 */
int logic_free(void) {
  HASH_CLEAR(hh, hash);
  addToCells = NULL;
  evolve = NULL;
  toggle_mod = -1;
  free(save_cells);
  save_cells_s = 0;
  return 1;
}

/**
 * @brief function that toggles the value at coords (i,j). E.g from 0->1, 1->2
 * or 2->0;
 */
int toggleAt(int i, int j) {
  Cell *c;

  if (!(c = get(i, j))) {
    insert(i, j, 1, 0);
    return 1;
  }

  int val = c->val = (c->val + 1) % toggle_mod;
  if (!c->val)
    deleter(c);
  return val;
}

/**
 * @brief function that destroys cell at coords(i,j);
 */
void deleteAt(int i, int j) {
  Cell *c;

  if ((c = get(i, j)))
    deleter(c);
}

/**
 * @brief function that sets value(val) at coords(i,j);
 */
void setAt(int i, int j, int val) {
  Cell *c;

  if ((c = get(i, j)) != NULL)
    c->val = val;
  else
    insert(i, j, val, 0);
}

/**
 * @brief functiong that returns value of a cell at given coords.
 */
int getAt(int i, int j) {
  Cell *c = get(i, j);
  return ((c) ? c->val : 0);
}

void setPosition(int i, int j) {
  pos_y = i;
  pos_x = j;
}

void saveCell(int i, int j) {
  Cell *c;

  if ((c = get(i, j))) {
    if (save_cells_s == save_cells_sm) {
      Cell **t;
      save_cells_sm *= 2;
      MEM_CHECK(t = realloc(save_cells, save_cells_sm));
      save_cells = t;
    }

    save_cells[save_cells_s++] = c;
  }
}
