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

struct T {
  WINDOW *win;
  T       c1;
  T       c2;
  T       sibiling;

  int param[4];
  int mod[3];
};

void WINDOW_init(WINDOW *win) {
  window_settings(win);
  box(win, ACS_VLINE, ACS_HLINE);
  wrefresh(win);
}

void WINDOW_new(T self) {
  self->win = newwin(H(self), W(self), Y(self), X(self));
  WINDOW_init(self->win);
}

T window_new(void) {
  T self = malloc(sizeof(*self));
  self->win = NULL;
  self->c1 = NULL;
  self->c2 = NULL;
  self->sibiling = NULL;
  H(self) = 0;
  W(self) = 0;
  Y(self) = 0;
  X(self) = 0;
  return self;
}

T       window_sibiling(T self) { return self->sibiling; }
int     window_height(T self) { return H(self) - 2; }
int     window_wight(T self) { return W(self) - 2; }
WINDOW *window_win(T self) { return self->win; }

T window_init(T self) {

  self->win = stdscr;
  box(self->win, ACS_VLINE, ACS_HLINE);
  wrefresh(self->win);
  H(self) = LINES;
  W(self) = COLS;
  return self;
}

void window_free(T self) {
  if (self == NULL)
    return;
  window_free(self->c1);
  window_free(self->c2);
  delwin(self->win);
  free(self);
}

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

T window_split(T self, int hor, int a, int b) {
  self->c1 = window_new();
  self->c2 = window_new();

  self->c1->sibiling = self->c2;
  self->c2->sibiling = self->c1;

  self->mod[0] = hor;
  self->mod[1] = a;
  self->mod[2] = b;

  window_calc_children(self);

  WINDOW_new(self->c1);
  WINDOW_new(self->c2);

  return self->c1;
}

T window_center(T self, int tmp, int h, int w) {
  self->c1 = window_new();
  self->c2 = NULL;

  self->mod[0] = tmp;
  self->mod[1] = h;
  self->mod[2] = w;

  window_calc_children(self);
  WINDOW_new(self->c1);

  return self->c1;
}

void window_update_children(T self) {
  if (self == NULL || self->c1 == NULL)
    return;

  window_calc_children(self);

  delwin(self->c1->win);
  WINDOW_new(self->c1);
  window_update_children(self->c1);

  if (self->c2 != NULL) {
    delwin(self->c2->win);
    WINDOW_new(self->c2);
    window_update_children(self->c2);
  }
}

void window_clear(T self) {
  wclear(self->win);
  WINDOW_init(self->win);
}

void window_settings(WINDOW *win) { keypad(win, TRUE); }

void wcenter_horizontal(T window, int y, int n) {
  wmove(window_win(window), y, (W(window) - n - 2) / 2 + 1);
}

int wcenter_vertical(T window, int n) { return (H(window) - n - 2) / 2 + 1; }

void cursor_offset(WINDOW *win, int oy, int ox) {
  int y, x;
  getyx(win, y, x);
  wmove(win, y + oy, x + ox);
}

#undef T
