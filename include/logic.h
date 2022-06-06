#ifndef LOGIC_H
#define LOGIC_H

#include "uthash.h"

#define MAX_SIZE 101

typedef struct Cell_cord {
  int row;
  int col;
} Cell_cord;

typedef struct Cell {
  Cell_cord      cord;
  unsigned char  val;
  UT_hash_handle hh;
} Cell;

extern Cell *hash;

extern char *evolution_names[];
extern int   evolution_cells[];
extern int   evolution_size, evolve_index;

extern int pos_y, pos_x;

extern Cell **save_cells;
extern int    save_cells_s;

int  logic_init(int isWrapping, int index);
int  evolution_init(int index);
void do_evolution(int steps);
int  logic_free(void);
int  toggleAt(int i, int j);
int  getAt(int i, int j);
void deleteAt(int i, int j);
void saveCell(int i, int j);
void setPosition(int i, int j);
void setAt(int i, int j, int val);

#endif
