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
extern mmask_t  mbitmask;

typedef int (*input_f)(int);

/*
 * @breaf A item in a menu
 *
 * It's intended to be used with display_menu().
 */
struct menu_T {
  void (*callback)(char *, int); ///< function called when item is selected
  char *name;                    ///< name of the menu item
};

/*
 * @breaf A item in a interactive menu
 *
 * It's intended to be used with display_imenu().
 */
struct imenu_T {
  char   *message; ///< prompt for the user
  int     size; ///< max number of characters required
  input_f crit; ///< function that check the validity of a character
  char   *buffer; ///< place where read character are stored
};

int input(WINDOW *win, char *buffer, int size, input_f crit);

int display_start(void);
int display_stop(void);

void display_menu(window_T wind, char *name, struct menu_T *items, int size,
                  int title);
int  display_imenu(window_T wind, struct imenu_T *items, int size);

void display_patterns(window_T wind);
void handle_winch(int sig);
void display_state_set(int i, int j, int val);

#endif
