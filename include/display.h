#ifndef DISPLAY_H
#define DISPLAY_H

#include "window.h"

#define CHAR_BLANK  "  "
#define CHAR_CURSOR "<>"
#define CHAR_CIRCLE "\u26AB"
#define CHAR_SQUARE "\u2B1B"
#define CHAR_ACTIVE CHAR_CIRCLE

#ifndef NO_UNICODE
#define print_cell(win, blank)                                                 \
  if (val)                                                                     \
    waddstr(win, CHAR_ACTIVE);                                                 \
  else                                                                         \
    waddstr(win, blank);
#else
#define print_cell(win, blank) waddstr(win, blank);
#endif

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
extern int wrap;

int input(WINDOW *win, char *buffer, int size, input_f crit);

int display_start(void);
int display_stop(void);

void display_menu(window_T wind, char *name, struct menu_T *items, int size,
                  int title);
int  display_imenu(window_T wind, struct imenu_T *items, int size);
void display_status(window_T wind, unsigned long int generation, int gen_step,
                    int wrap, int height, int wight, int play, int dt,
                    int cursor_y, int cursor_x);

void display_patterns(window_T wind);
void handle_winch(int sig);
void display_state_set(int i, int j, int val);

#endif
