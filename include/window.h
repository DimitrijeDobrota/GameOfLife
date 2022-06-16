#ifndef WINDOW_H
#define WINDOW_H

#include <curses.h>

#define window_T T
typedef struct T *T;

// General

T window_new(void);
T window_init(T self);
T window_split(T self, int hor, int a, int b, char *name1, char *name2);
T window_center(T self, int h, int w, char *name);

WINDOW *window_win_new(T self);

int  window_clicked(T self, int y, int x);
void window_clear(T self);
void window_clear_noRefresh(T self);
void window_free(T self);
void window_settings(WINDOW *win);
void window_unsplit(T self);
void window_update_children(T self);

// Setters and Gettern

T window_sibiling(T self);

WINDOW *window_win(T self);

int  window_height(T self);
int  window_wight(T self);
int  window_x(T self);
int  window_y(T self);
void window_set_title(T self, char *title);

// Help

int  wcenter_vertical(T window, int n);
void cursor_offset(WINDOW *win, int oy, int ox);
void wcenter_horizontal(T window, int y, int n);

#undef T
#endif
