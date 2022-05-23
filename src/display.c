#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "logic.h"
#include "utils.h"
#include "window.h"

#define center_vertical(n)      wcenter_vertical(MAIN_W, n);
#define center_horizontal(y, n) wcenter_horizontal(MAIN_W, y, n);

#define CHAR_CIRCLE     L'\u26AB'
#define CHAR_SQUARE     L'\u2B1B'
#define CHAR_SQUARED    L'\u2B1C'
#define CHAR_CIRCLE_DOT L'\u2299'

window_T MAIN_w = NULL;

int input(WINDOW *win, char *buffer, int size, input_f crit) {
  int CLINES = LINES, CCOLS = COLS;
  int ch, read = strlen(buffer);

  while ((ch = getch()) != '\n') {
    switch (ch) {
    case 27:
      buffer[read] = '\0';
      return 100;
    case KEY_BACKSPACE:
    case KEY_LEFT:
      if (read > 0) {
        cursor_offset(win, 0, -1);
        wprintw(win, " ");
        cursor_offset(win, 0, -1);
        read--;
      }
      break;
    case KEY_UP:
      buffer[read] = '\0';
      return -1;
    case KEY_DOWN:
      buffer[read] = '\0';
      return +1;
    default:
      if (read < size && crit && crit(ch)) {
        wprintw(win, "%c", ch);
        buffer[read++] = ch;
      }
      break;
    }
    if (is_term_resized(CLINES, CCOLS)) {
      buffer[read] = '\0';
      return -100;
    }
    wrefresh(win);
  }
  buffer[read] = '\0';
  return 0;
}

int display_imenu(window_T wind, struct imenu_T *items, int size) {
  WINDOW *win;
  int     y_offset;
  int     current = 0;

  for (int i = 0; i < size; i++)
    if (!items[i].buffer)
      items[i].buffer = calloc(5, sizeof(char));

  int maxi = 0, len = 0;
  for (int i = 0; i < size; i++)
    if ((len = strlen(items[i].message)) > maxi)
      maxi = len;

  curs_set(1);
  window_clear(wind);
redraw:;
  win = window_win(wind);
  y_offset = wcenter_vertical(wind, size);

  for (int i = 0; i < size; i++) {
    wcenter_horizontal(wind, y_offset + 2 * i, 20);
    wprintw(win, "%*s: %s", -maxi, items[i].message, items[i].buffer);
  }
  wrefresh(win);

  while (TRUE) {
    for (int i = 0; i < size; i++) {
      if (current != i)
        continue;

      wcenter_horizontal(wind, y_offset + 2 * i, 20);
      wprintw(win, "%*s: %s", -maxi, items[i].message, items[i].buffer);
      switch (input(win, items[i].buffer, items[i].size, items[i].crit)) {
      case -1:
        current--;
        break;
      case 0:
        if (++current == size) {
          curs_set(0);
          return 1;
        }
        break;
      case 1:
        current++;
        break;
      case 100:
        curs_set(0);
        return 0;
      case -100:
        HANDLE_RESIZE;
        goto redraw;
      }
      CLAMP(current, 0, size - 1);
    }
  }

  curs_set(0);
}

void display_menu(window_T wind, struct menu_T *items, int size) {
  WINDOW *win;
  int     current = 0;

  int maxi = 0, len = 0;
  for (int i = 0; i < size; i++)
    if ((len = strlen(items[i].name)) > maxi)
      maxi = len;

redraw:;
  win = window_win(wind);
  int CLINES = LINES, CCOLS = COLS;
  int y_offset = wcenter_vertical(wind, size * 2);
  while (TRUE) {
    CLAMP(current, 0, size - 1);

    for (int i = 0; i < size; i++) {
      wattrset(win, COLOR_PAIR(i == current ? 1 : 0));
      wcenter_horizontal(wind, y_offset + i * 2, maxi);
      wprintw(win, "%s", items[i].name);
    }
    wrefresh(win);

    while (TRUE) {
      int c = getch();
      if (c == 'k' || c == KEY_UP) {
        current--;
        break;
      } else if (c == 'j' || c == KEY_DOWN) {
        current++;
        break;
      } else if (c == '\n') {
        wattrset(win, COLOR_PAIR(0));
        window_clear(wind);
        items[current].callback(wind, items[current].name, current);
        /* window_clear(wind); */
        return;
      } else if (c == 27)
        return;
      if (is_term_resized(CLINES, CCOLS)) {
        HANDLE_RESIZE;
        goto redraw;
      }
    }
  }
}

#define y_at(y) (y + screen_offset_y + h) % h + 1
#define x_at(x) (x + screen_offset_x + w) % w + 1

void display_game(window_T wind, cell **mat, int w, int h, int screen_offset_y,
                  int screen_offset_x, int *cursor_offset_y,
                  int *cursor_offset_x) {
  WINDOW *win = window_win(wind);
  wattrset(win, COLOR_PAIR(0));

  int ph = window_height(wind), pw = window_wight(wind) / 2;

#ifdef _WIN32
  window_clear(wind);
#endif
  for (int i = 0; i < ph; i++) {
    wmove(win, i + 1, 1);
    for (int j = 0; j < pw; j++) {
      wattrset(win, COLOR_PAIR((mat[y_at(i)][x_at(j)]) + 2));

      if (mat[y_at(i)][x_at(j)])
        waddstr(win, "\u26AB");
      else
        waddstr(win, "  ");
    }
  }

  CLAMP(*cursor_offset_y, 0, ph - 1);
  CLAMP(*cursor_offset_x, 0, pw - 1);

  wmove(win, *cursor_offset_y + 1, *cursor_offset_x * 2 + 1);
  wattrset(win, COLOR_PAIR(
                    (mat[y_at(*cursor_offset_y)][x_at(*cursor_offset_x)]) + 5));

  if (mat[y_at(*cursor_offset_y)][x_at(*cursor_offset_x)])
    waddstr(win, "\u26AB");
  else
    waddstr(win, "  ");

  wrefresh(win);
}

#define LMAX   8
#define LEFT   0
#define CENTER 1
#define RIGHT  2

void curses_start(void) {
  initscr();
  window_settings(stdscr);

  start_color();
  use_default_colors();

  curs_set(0);
  noecho();
  nodelay(stdscr, 1);

  init_pair(0, COLOR_WHITE, -1);
  init_pair(1, COLOR_RED, -1);

  init_pair(2, COLOR_WHITE, -1);
  init_pair(3, COLOR_WHITE, -1);
  init_pair(4, COLOR_RED, -1);

  init_pair(5, COLOR_WHITE, COLOR_CYAN);
  init_pair(6, COLOR_WHITE, COLOR_CYAN);
  init_pair(7, COLOR_RED, COLOR_CYAN);
}

void curses_stop(void) {
  window_free(MAIN_w);
  endwin();
}

void handle_winch(int sig) {
  endwin();
  refresh();
  clear();

  window_init(MAIN_w);
  window_update_children(MAIN_w);
}

int display_start(void) {
  /* #ifndef _WIN32 */
  /*   signal(SIGWINCH, handle_winch); */
  /* #endif */

  curses_start();
  MAIN_w = window_init(window_new());

#ifdef _WIN32
  resize_term(0, 0);
  handle_winch(10);
#endif

  return 1;
}

int display_stop(void) {
  curses_stop();
  return 1;
}
