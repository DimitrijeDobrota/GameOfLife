/**
 * @file display.h
 * @author Dimitrije Dobrota
 * @date 19 June 2022
 * @brief Methods and types for displaying UI
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "window.h"

#ifdef _WIN32
#define is_term_resized(a, b) is_termresized()
#define HANDLE_RESIZE                                                          \
  {                                                                            \
    resize_term(0, 0);                                                         \
    handle_winch(10);                                                          \
  }
#else
#define HANDLE_RESIZE                                                          \
  { handle_winch(10); }
#endif

/// Character representing dead cell
#define CHAR_BLANK "  "

/// Character representing cursor
#define CHAR_CURSOR "<>"

/// Character representing a living cell with a circle
#define CHAR_CIRCLE "\u26AB"

/// Character representing a living cell with a square
#define CHAR_SQUARE "\u2B1B"

/// Active representation of a living cell
#define CHAR_ACTIVE CHAR_CIRCLE

/// Way of printing a cell based on Unicode support
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

/**
 * @brief A item in a menu
 *
 * It's intended to be used with display_menu().
 */
struct menu_T {
  void (*callback)(char *, int); ///< function called when item is selected
  char *name;                    ///< name of the menu item
};

/**
 * @brief A item in a interactive menu
 *
 * It's intended to be used with display_imenu().
 */
struct imenu_T {
  char   *message; ///< prompt for the user
  int     size;    ///< max number of characters required
  input_f crit;    ///< function that check the validity of a character
  char   *buffer;  ///< place where read character are stored
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
