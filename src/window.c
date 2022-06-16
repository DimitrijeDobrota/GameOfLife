/**
 * @file window.c
 * @author Dimitrije Dobrota
 * @date 9 June 2022
 * @brief This file contains the implementation of the window interface
 *
 * This file handles the creation and deletion process of nodes in the binary
 * tree of windows used for recalculating the ncurses WINDOW position in
 * dimension when the terminal has been resized. It's possible to "split" the
 * current window in two using a given ration or fixed size, as well as create
 * a centered subwindow. It makes some of the tree data accessible via function
 * so that other functions can know about the sizes and adapt to them.
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "window.h"

#define H(c) c->param[0]
#define W(c) c->param[1]
#define Y(c) c->param[2]
#define X(c) c->param[3]

#define window_T T
typedef struct T *T;

/**
 * @brief A node in a binary tree of windows
 *
 * window_T contains all of the information about the size and dimensions of
 * ncurses WINDOW, as well as all the information on how to recalculate
 * children when a terminal resize occurs.
 */
struct window_T {
  WINDOW *win;     ///< ncurses WINDOW* that represents current window_T
  T       c1;      ///< left child, NULL if none
  T       c2;      ///< right child, NULL if none
  T       sibling; ///< sibling, NULL if none

  int param[4]; ///<  windows size and dimension, refer to macros
  int mod[3];   ///< Information an recreating children

  char *title; ///< Window title to display, NULL if none
};

/**
 * @breif Apply settings, draw a border, print title for a give window
 *
 * @param title: title to be prinded unless NULL
 */
void WINDOW_init(WINDOW *win, char *title) {
  window_settings(win);
  box(win, ACS_VLINE, ACS_HLINE);
  if (title)
    mvwprintw(win, 0, 1, " %s ", title);
}

/**
 * @brief create new window_T
 *
 * Allocate the memory for a new window_T and set its fields to the default
 * value. Should be the only way new window_T objects are created
 */
T window_new(void) {
  T self;

  MEM_CHECK(self = malloc(sizeof(*self)));
  self->win = NULL;
  self->c1 = NULL;
  self->c2 = NULL;
  self->sibling = NULL;
  self->title = NULL;
  H(self) = 0;
  W(self) = 0;
  Y(self) = 0;
  X(self) = 0;
  return self;
}

/// Getter: return the sibling of the current window
T window_sibiling(T self) { return self->sibling; }

/// Getter: return the height of the current window
int window_height(T self) { return H(self) - 2; }

/// Getter: return the width of a current window
int window_wight(T self) { return W(self) - 2; }

/// Getter: return the x coordinate of the current window
int window_x(T self) { return X(self); }

/// Getter: return the y coordinate of the current window
int window_y(T self) { return Y(self); }

/// Getter: return the ncurses WINDOW* of the current window
WINDOW *window_win(T self) { return self->win; }

/// Setter: set the title of the current window
void window_set_title(T self, char *title) { self->title = title; }

/**
 * @brief create new ncurses WINDOW to the specification with default settings
 *
 * New ncruses WINDOW is created with newwin and it should be deleted with
 * delwin. Setting are applied with the call to WINDOW_INIT
 */
WINDOW *window_win_new(T self) {
  WINDOW *win = newwin(H(self), W(self), Y(self), X(self));
  WINDOW_init(win, self->title);
  wrefresh(win);
  return win;
}

/**
 * @brief Initialize the window to be used as main
 *
 * This function will use the ncurses interface directly to set the screen size
 * to match the whole terminal. This window object should always be used as a
 * root of the tree and under no condition should it be used as a child of
 * other window.
 *
 * There can be multiple instances of windows initialized with this function
 */
T window_init(T self) {
  self->win = stdscr;
  WINDOW_init(stdscr, self->title);
  wrefresh(self->win);
  H(self) = LINES;
  W(self) = COLS;
  return self;
}

/**
 * @brief Free the window and it's children recursively
 *
 * Free the object itself and ncurses WINDOW* with delwin
 */
void window_free(T self) {
  if (self == NULL)
    return;
  window_free(self->c1);
  window_free(self->c2);
  delwin(self->win);
  free(self);
}

/**
 * @brief Delete all children and subchildren of a window
 */
void window_unsplit(T self) {
  window_free(self->c1);
  window_free(self->c2);
  self->c1 = NULL;
  self->c2 = NULL;
}

/**
 * @brief Given the window calculate the position and dimensions of it's
 * children
 *
 * This function takes care of calculation for two windows in a given ration or
 * fixed size, and single subwindow.
 * If a given windows has 2 children then they are split with the following
 * rule:
 * - If mod[0] is 0, two created windows will be side by side
 * - If mod[0] is not 0, two created windows will be one on top of the other
 * - If mod[1] and mod[2] are 0, nothing happens
 * - If mod[1] or mod[2] is 0, the window with a non zero value will be of a
 *   fixed size, while the other will fill the rest of the screen
 * If a given windows has 1 child then:
 * - Child dimensions will be mod[1] x mod[2] or made to fit in it's parent
 */
void window_calc_children(T self) {
  T c1, c2, f, nf;
  c1 = self->c1;
  c2 = self->c2;
  int fixed = 0, *fv;
  int hor = self->mod[0], a = self->mod[1], b = self->mod[2];

  if (c2 != NULL) {
    if (a == 0 && b == 0)
      return;

    if (a == 0) {
      f = c2;
      nf = c1;
      fv = &b;
      fixed = 1;
    } else if (b == 0) {
      f = c1;
      nf = c2;
      fv = &a;
      fixed = 1;
    }

    if (hor) {
      W(c1) = W(c2) = W(self) - 2;

      if (!fixed) {
        H(c1) = ((H(self) - 2) / (a + b)) * a;
      } else {
        H(f) = *fv;
      }
      H(nf) = (H(self) - 2) - H(f);

      X(c1) = X(c2) = X(self) + 1;
      Y(c1) = Y(self) + 1;
      Y(c2) = Y(self) + H(c1) + 1;
    } else {
      H(c1) = H(c2) = H(self) - 2;

      if (!fixed) {
        W(c1) = ((W(self) - 2) / (a + b)) * a;
      } else {
        W(f) = *fv;
      }
      W(nf) = (W(self) - 2) - W(f);

      Y(c1) = Y(c2) = Y(self) + 1;
      X(c1) = X(self) + 1;
      X(c2) = X(self) + W(c1) + 1;
    }
  } else {
    H(c1) = ACLAMP(a + 2, 0, H(self));
    W(c1) = ACLAMP(b + 2, 0, W(self));
    Y(c1) = Y(self) + (H(self) - H(c1)) / 2;
    X(c1) = X(self) + (W(self) - W(c1)) / 2;
  }
}

/**
 * @brief Split the window in a given ration into two windows with a name
 *
 * @param hor: should the split line be horizontal or verical
 * @param a: ration of the first window
 * @param b: ration of the second window
 * @param name1: name of the first window
 * @param name2: name of the second window
 *
 * @see window_calc_children to see how the ratios are used
 */
T window_split(T self, int hor, int a, int b, char *name1, char *name2) {
  if (self->c1)
    window_free(self->c1);
  if (self->c2)
    window_free(self->c2);

  self->c1 = window_new();
  self->c2 = window_new();

  self->c1->sibling = self->c2;
  self->c2->sibling = self->c1;

  self->mod[0] = hor;
  self->mod[1] = a;
  self->mod[2] = b;

  window_calc_children(self);

  self->c1->title = name1;
  self->c2->title = name2;

  self->c1->win = window_win_new(self->c1);
  self->c2->win = window_win_new(self->c2);

  return self->c1;
}

/**
 * @brief Create a subwindow that will be of a given size or made to fit it's
 * parent
 *
 * @param h: desired height of a new window_T
 * @param w: desired width of a new window_T
 * @param name: name of the new window_T
 */
T window_center(T self, int h, int w, char *name) {
  self->c1 = window_new();
  self->c2 = NULL;

  self->mod[0] = -1;
  self->mod[1] = h;
  self->mod[2] = w;

  window_calc_children(self);

  self->c1->title = name;
  self->c1->win = window_win_new(self->c1);

  return self->c1;
}

/**
 * @brief Recursevly recalculate the size of each child and make new ncurses
 * WINDOW
 */
void window_update_children(T self) {
  if (self == NULL || self->c1 == NULL)
    return;

  window_calc_children(self);

  delwin(self->c1->win);
  self->c1->win = window_win_new(self->c1);
  window_update_children(self->c1);

  if (self->c2 != NULL) {
    delwin(self->c2->win);
    self->c2->win = window_win_new(self->c2);
    window_update_children(self->c2);
  }
}

/**
 * @brief Clear the current window, reset the setting, keep the border call
 * ncurses refresh()
 */
void window_clear(T self) {
  werase(self->win);
  WINDOW_init(self->win, self->title);
  wrefresh(self->win);
}

/**
 * @brief Same as window_clear but without calling ncurses refresh()
 *
 * @see window_clear
 */
void window_clear_noRefresh(T self) {
  werase(self->win);
  WINDOW_init(self->win, self->title);
}

/**
 * @brief Default setting for each window
 */
void window_settings(WINDOW *win) {
  keypad(win, TRUE);
  wattrset(win, COLOR_PAIR(0));
}

/**
 * @brief Check if cordinates are in the window
 */
int window_clicked(T self, int y, int x) {
  if (x <= X(self) || y <= Y(self))
    return 0;

  if (x >= X(self) + W(self) || y >= Y(self) + H(self))
    return 0;

  return 1;
}

/**
 * @brief Move the cursor to the given why position and x position so that the
 * text of len n will be centered
 *
 * @param y: y position
 * @n lenfth of the text to be centered
 */
void wcenter_horizontal(T window, int y, int n) {
  wmove(window->win, y, (W(window) - n - 2) / 2 + 1);
}

/**
 * @brief Return the y position of the cursor so that n lines will be centered
 *
 * @param n: number of lines to be centered
 */
int wcenter_vertical(T window, int n) { return (H(window) - n - 2) / 2 + 1; }

#undef T
