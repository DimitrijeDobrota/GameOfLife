#ifndef WINDOW_H
#define WINDOW_H

#include <curses.h>

#define window_T T
typedef struct T *T;

T    window_new(void);
void window_free(T self);
T    window_init(T self);
T    window_split(T self, int hor, int a, int b);
T    window_center(T self, int tmp, int h, int w);
void window_update_children(T self);

T       window_sibiling(T self);
int     window_height(T self);
int     window_wight(T self);
WINDOW *window_win(T self);
void    window_settings(WINDOW *win);
void    window_clear(T self);

void wcenter_horizontal(T window, int y, int n);
int  wcenter_vertical(T window, int n);
void cursor_offset(WINDOW *win, int oy, int ox);

#undef T
#endif
