// This is not a final version of the code
// While I've cleaned this file up,
// all of the game logic is still here

#include <ctype.h>
#include <curses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include "display.h"
#include "file.h"
#include "utils.h"
#include "window.h"

extern window_T MAIN_w;

WINDOW *game_w, *board_w, *status_w;
int     top_space = 5;

void show(window_T wind, unsigned **univ, int w, int h, int y, int x) {
  WINDOW *win = window_win(wind);
  wattrset(win, COLOR_PAIR(0));

  int ph = h, pw = w, mh, mw;
  mh = window_height(wind);
  mw = window_wight(wind) / 2;
  CLAMP(ph, 0, mh);
  CLAMP(pw, 0, mw);

  int top = wcenter_vertical(wind, ph);
  for (int i = 0; i < ph; i++) {
    wcenter_horizontal(wind, top + i, pw * 2);
    for (int j = 0; j < pw; j++) {
#ifdef _WIN32
      wattrset(win,
               COLOR_PAIR((univ[(i + y + h) % h][(j + x + w) % w]) ? 2 : 3));
      wprintw(win, "  ");
#else
      if (univ[(i + y + h) % h][(j + x + w) % w])
        wprintw(win, "%lc ", (wchar_t)L'\u23FA');
      else
        wprintw(win, "  ");
        /* wprintw(win, "%lc", (wchar_t)L'\u2B1B'); */
        /* wprintw(win, "  ", (wchar_t)L'\u2B1C'); */
#endif
    }
  }
  wrefresh(win);
}

void evolve(unsigned **univ, unsigned **new, int w, int h, int step) {
  do {
    for (int i = 0; i < h; i++)
      for (int j = 0; j < w; j++) {
        int n = 0;
        for (int y1 = i - 1; y1 <= i + 1; y1++)
          for (int x1 = j - 1; x1 <= j + 1; x1++)
            if (univ[(y1 + h) % h][(x1 + w) % w])
              n++;

        if (univ[i][j])
          n--;
        new[i][j] = (n == 3 || (n == 2 && univ[i][j]));
      }
    unsigned **t = univ;
    univ = new;
    new = t;
  } while (--step);
  unsigned **t = univ;
  univ = new;
  new = t;

  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
      univ[i][j] = new[i][j];
}

void game(window_T wind, int w, int h) {
  if (w <= 0 || h <= 0) {
    h = window_height(wind);
    w = window_wight(wind);
    w /= 2;
  }

  unsigned **univ;
  univ = malloc(h * sizeof(unsigned *));
  for (int i = 0; i < h; i++) {
    univ[i] = malloc(w * sizeof(unsigned));
    for (int j = 0; j < w; j++)
      univ[i][j] = rand() < RAND_MAX / 7 ? 1 : 0;
  }

  unsigned **new = malloc(h * sizeof(unsigned *));
  for (int i = 0; i < h; i++) {
    new[i] = malloc(w * sizeof(unsigned));
  }

  int oy = 0, ox = 0;
  int in = 3;
  int gen_step = 1;

  window_clear(wind);
redraw:;
  /* window_clear(wind); */
  int CLINES = LINES, CCOLS = COLS;
  while (TRUE) {
    show(wind, univ, w, h, oy, ox);
    evolve(univ, new, w, h, gen_step);
    for (int i = 0; i < in; i++) {
      int c = getch();
      switch (c) {
      case KEY_A1:
      case KEY_C1:
      case KEY_END:
      case KEY_HOME:
      case KEY_LEFT:
        ox--;
        break;
      case KEY_A3:
      case KEY_C3:
      case KEY_NPAGE:
      case KEY_PPAGE:
      case KEY_RIGHT:
        ox++;
        break;
      }
      switch (c) {
      case KEY_A1:
      case KEY_A3:
      case KEY_HOME:
      case KEY_PPAGE:
      case KEY_UP:
        oy--;
        break;
      case KEY_C1:
      case KEY_C3:
      case KEY_DOWN:
      case KEY_END:
      case KEY_NPAGE:
        oy++;
      }
      break;
      switch (c) {
      case 27:
        return;
      }
    }
    ox = (ox + w) % w;
    oy = (oy + h) % h;

    flushinp();

    usleep(200000);
    if (is_term_resized(CLINES, CCOLS)) {
      HANDLE_RESIZE;
      goto redraw;
    }
  }
}

struct imenu_T imenu_items[] = {
    {"Unesi broj redova", 4, isdigit, NULL},
    {"Unesi broj kolona", 4, isdigit, NULL},
};
int imenu_items_s = sizeof(imenu_items) / sizeof(struct imenu_T);

void settings(window_T wind, char *pass) {
  if (display_imenu(wind, imenu_items, imenu_items_s)) {
    int row = atoi(imenu_items[0].buffer);
    int column = atoi(imenu_items[1].buffer);
    game(wind, row, column);
  }

  for (int i = 0; i < imenu_items_s; i++)
    free(imenu_items[i].buffer);
}

void load(window_T wind, char *pass) {
  char **buffer;
  int    n;

  load_files();
  n = file_select("all", &buffer);

  struct menu_T *file_items = malloc(n * sizeof(struct menu_T));
  for (int i = 0; i < n; i++) {
    file_items[i].name = buffer[i];
    file_items[i].callback = load_file;
  }
  free(buffer);

  display_menu(wind, file_items, n);
  game(wind, 0, 0);

  free(file_items);
  free_files();
}

void exitp(window_T wind, char *pass) {
  display_stop();
  exit(0);
}

struct menu_T mode_items[] = {
    {settings,         "Normalan"},
    {settings,   "Koegzistencija"},
    {settings, "Predator i pljen"},
    {settings,            "Virus"},
    {settings,      "Nepoznanost"},
};
int mode_items_s = sizeof(mode_items) / sizeof(struct menu_T);

void mode_select(window_T wind, char *pass) {
  display_menu(wind, mode_items, mode_items_s);
}

struct menu_T menu_items[] = {
    {mode_select, "Start"},
    {       load,  "Load"},
    {      exitp,  "Exit"}
};
int menu_items_s = sizeof(menu_items) / sizeof(struct menu_T);

int state = 0;

int main(void) {
  setlocale(LC_ALL, "");
  if (!display_start()) {
    printf("Couldn't start the display!\n");
  }
  if (!file_setup()) {
    printf("File setup error\n");
  }

  T t1, t2;
  t1 = window_split(MAIN_w, 1, 5, 0);
  t2 = window_sibiling(t1);

  while (TRUE) {
    display_menu(t2, menu_items, menu_items_s);
    window_clear(t2);
  }

  if (!display_stop()) {
    printf("Couldn't stop the display!\n");
  }

  return 0;
}
