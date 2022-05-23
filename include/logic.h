#ifndef LOGIC_H
#define LOGIC_H

#define cell unsigned char

extern cell **mat;
extern char  *evolution_names[];
extern int    evolution_cells[];
extern int    evolution_size;

int  logic_init(int w, int h);
int  evolution_init(int index);
void do_evolution(int steps);
int  logic_free(void);

#endif
