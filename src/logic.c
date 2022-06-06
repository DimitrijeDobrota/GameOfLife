#include <stdio.h>
#include <stdlib.h>

#include "logic.h"
#include "utils.h"

Cell *hash = NULL;

void deleter(Cell *c) {
  HASH_DEL(hash, c);
  free(c);
}

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
    c = malloc(sizeof(Cell));
    c->cord.row = row;
    c->cord.col = col;
    c->val = val;
    HASH_ADD(hh, hash, cord, sizeof(Cell_cord), c);
  }
  c->val += mod;
}

int WIDTH, HEIGHT;
int isExpanding;

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
      int a = (k + HEIGHT) % HEIGHT;
      int b = (l + WIDTH) % WIDTH;
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

/* Evolutions */
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

void do_evolution(int steps) {
  while (steps--) {
    evolve();
  }
}

int logic_init(int w, int h, int isWrapping) {
  WIDTH = w;
  HEIGHT = h;
  addToCells = addition_modes[isWrapping];
  save_cells_sm = 100;
  save_cells = malloc(save_cells_sm * sizeof(struct Cell *));
  save_cells_s = 0;

  return 1;
}

int evolution_init(int index) {
  evolve_index = index;
  evolve = evolution_modes[index];
  toggle_mod = evolution_cells[index];
  return 1;
}

int logic_free(void) {
  HASH_CLEAR(hh, hash);
  addToCells = NULL;
  evolve = NULL;
  toggle_mod = -1;
  free(save_cells);
  save_cells_s = 0;
  return 1;
}

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

void deleteAt(int i, int j) {
  Cell *c;

  if ((c = get(i, j)))
    deleter(c);
}

void setAt(int i, int j, int val) {
  Cell *c;

  if ((c = get(i, j)) != NULL)
    c->val = val;
  else
    insert(i, j, val, 0);
}

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
      t = realloc(save_cells, save_cells_sm);
      if (!t)
        exit(1);
      save_cells = t;
    }

    save_cells[save_cells_s++] = c;
  }
}
