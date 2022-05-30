#ifndef LOGIC_H
#define LOGIC_H

extern char *evolution_names[];
extern int   evolution_cells[];
extern int   evolution_size;

int logic_init(int w, int h, int isWrapping);
int evolution_init(int index);
int do_evolution(int steps);
int logic_free(void);
void toggleAt(int i, int j)
void deleteAt(int i, int j);
int getNext(int *row, int *col, int *value, int reset);

#endif
