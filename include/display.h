#ifndef DISPLAY_H
#define DISPLAY_H

#include "window.h"

extern window_T MAIN_w;

typedef int (*input_f)(int);

struct menu_T {
  void (*callback)(char *, int);
  char *name;
};

struct imenu_T {
  char   *message;
  int     size;
  input_f crit;
  char   *buffer;
};

extern int screen_offset_x, screen_offset_y;

int input(WINDOW *win, char *buffer, int size, input_f crit);

int display_start(void);
int display_stop(void);

void display_menu(window_T wind, char *name, struct menu_T *items, int size);
int  display_imenu(window_T wind, struct imenu_T *items, int size);
void display_game(WINDOW *win, int h, int w, int ph, int pw, int wrap);
int  display_select(window_T wind, int w, int h);
void display_status(window_T wind, unsigned long int generation, int gen_step,
                    int wrap, int height, int wight, int play, int dt,
                    int cursor_y, int cursor_x);
void display_cursor(WINDOW *win, int h, int w, int ph, int pw);

void handle_winch(int sig);

void display_state_set(int i, int j, int val);

#endif
