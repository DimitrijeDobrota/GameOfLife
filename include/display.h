#ifndef DISPLAY_H
#define DISPLAY_H

#include "window.h"
#include "logic.h"

extern window_T MAIN_w;

typedef int (*input_f)(int);

struct menu_T {
  void (*callback)(window_T, char *, int);
  char *name;
};

struct imenu_T {
  char   *message;
  int     size;
  input_f crit;
  char   *buffer;
};

int input(WINDOW *win, char *buffer, int size, input_f crit);

int display_start(void);
int display_stop(void);

void display_menu(window_T win, struct menu_T *items, int size);
int  display_imenu(window_T wind, struct imenu_T *items, int size);
void display_game(window_T wind, cell **mat, int w, int h, int screen_offset_y,
                  int screen_offset_x, int *cursor_offset_y,
                  int *cursor_offset_x);

void handle_winch(int sig);

#endif
