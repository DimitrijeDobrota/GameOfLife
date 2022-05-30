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

#define CHAR_BLANK      "  "
#define CHAR_CIRCLE     "\u26AB"
#define CHAR_SQUARE     "\u2B1B"
#define CHAR_SQUARED    "\u2B1C"
#define CHAR_CIRCLE_DOT "\u2299"

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
  y_offset = wcenter_vertical(wind, size * 2 - 1);

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

void display_menu(window_T wind, char *name, struct menu_T *items, int size) {
  WINDOW *win;
  int     current = 0;

  int maxi = 0, len = 0;
  for (int i = 0; i < size; i++)
    if ((len = strlen(items[i].name)) > maxi)
      maxi = len;

  window_set_title(wind, name);
  window_clear(wind);
redraw:;
  win = window_win(wind);
  int CLINES = LINES, CCOLS = COLS;
  int y_offset = wcenter_vertical(wind, size * 2 - 1);
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
      if (c == 'k' || c == 'w' || c == KEY_UP) {
        current--;
        break;
      } else if (c == 'j' || c == 's' || c == KEY_DOWN) {
        current++;
        break;
      } else if (c == '\n') {
        wattrset(win, COLOR_PAIR(0));
        items[current].callback(items[current].name, current);
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

#define y_at(y)        (y + screen_offset_y + h) % h + 1
#define x_at(x)        (x + screen_offset_x + w) % w + 1
#define at(y, x)       mat[y_at(y)][x_at(x)]
#define at_offset(off) mat[y_at(off##_y)][x_at(off##_x)]

#ifndef NO_UNICODE
#define print_cell(win, k, l, blank)                                           \
  if (at(k, l))                                                                \
    waddstr(win, CHAR_CIRCLE);                                                 \
  else                                                                         \
    waddstr(win, blank);
#else
#define print_cell(win, i, j, blank) waddstr(win, blank);
#endif

#define print_cells(win, start_i, end_i, start_j, end_j, color_offset, blank)  \
  for (int i = start_i; i < end_i; i++) {                                      \
    wmove(win, i + 1, 1 + start_j * 2);                                        \
    for (int j = start_j; j < end_j; j++) {                                    \
      wattrset(win, COLOR_PAIR((mat[y_at(i)][x_at(j)]) + color_offset));       \
      print_cell(win, i, j, blank);                                            \
    }                                                                          \
  }

int screen_offset_x, screen_offset_y;
int cursor_offset_x, cursor_offset_y;

void display_game(window_T wind, cell **mat, int h, int w, int ph, int pw) {
  WINDOW *win = window_win(wind);
#ifdef _WIN32
  window_clear(wind);
#endif
  print_cells(win, 0, ph, 0, pw, 2, CHAR_BLANK);

  wmove(win, cursor_offset_y + 1, cursor_offset_x * 2 + 1);
  wattrset(win, COLOR_PAIR(at_offset(cursor_offset) + 5));

  print_cell(win, cursor_offset_y, cursor_offset_x, "<>");

  wrefresh(win);
}

void display_select(window_T wind, cell **mat, int w, int h) {
  int current_offset_y = cursor_offset_y;
  int current_offset_x = cursor_offset_x;

redraw:;
  int CLINES = LINES, CCOLS = COLS;
  WINDOW *new = WINDOW_new(wind);
  WINDOW *win = window_win(wind);

  overlay(win, new);
  wrefresh(new);

  int ph = window_height(wind), pw = window_wight(wind) / 2;
  nodelay(stdscr, 0);
  while (TRUE) {
    int start_i = MIN(cursor_offset_y, current_offset_y);
    int end_i = MAX(cursor_offset_y, current_offset_y);
    int start_j = MIN(cursor_offset_x, current_offset_x);
    int end_j = MAX(cursor_offset_x, current_offset_x);

    print_cells(new, start_i, end_i + 1, start_j, end_j + 1, 8, CHAR_BLANK);
    wrefresh(new);

    if (is_term_resized(CLINES, CCOLS)) {
      flushinp();
      delwin(new);
      HANDLE_RESIZE;
      ph = window_height(wind), pw = window_wight(wind) / 2;
      display_game(wind, mat, w, h, ph, pw);
      goto redraw;
    }

    int c = getch();
    switch (c) {
    // offset selection
    case 'w':
    case 'W':
      current_offset_y--;
      break;
    case 's':
    case 'S':
      current_offset_y++;
      break;
    case 'a':
    case 'A':
      current_offset_x--;
      break;
    case 'd':
    case 'D':
      current_offset_x++;
      break;
    case '\n':
      save_pattern();

    // quit
    case 27:
    case 'q':
    case 'Q':
      nodelay(stdscr, 1);
      delwin(new);
      return;
    defalut:
      continue;
    }

    CLAMP(current_offset_y, 0, ph - 1);
    CLAMP(current_offset_x, 0, pw - 1);

    wclear(new);
    overlay(win, new);
  }
}

void display_status(window_T wind, unsigned long int gen, int gen_step,
                    int height, int wight, int play, int dt, int cursor_y,
                    int cursor_x) {
  WINDOW *win = window_win(wind);

  wmove(win, 1, 1);
  wprintw(win, " | %5s | ", play ? "play" : "pause");
  wprintw(win, "Size: %dx%d | ", height, wight);
  wprintw(win, "Generation: %10lu(+%d) | ", gen, gen_step);
  wprintw(win, "dt: %4dms | ", dt);
  wprintw(win, "Cursor: %4dx%-4d | ", cursor_y, cursor_x);
  wrefresh(win);
}

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

#ifndef NO_UNICODE
  init_pair(2, COLOR_WHITE, -1);
  init_pair(3, COLOR_WHITE, -1);
  init_pair(4, COLOR_RED, -1);

  init_pair(5, COLOR_WHITE, COLOR_CYAN);
  init_pair(6, COLOR_WHITE, COLOR_CYAN);
  init_pair(7, COLOR_RED, COLOR_CYAN);

  init_pair(8, COLOR_WHITE, COLOR_YELLOW);
  init_pair(9, COLOR_WHITE, COLOR_YELLOW);
  init_pair(10, COLOR_RED, COLOR_YELLOW);
#else
  init_pair(2, COLOR_WHITE, -1);
  init_pair(3, COLOR_WHITE, COLOR_WHITE);
  init_pair(4, COLOR_RED, COLOR_RED);

  init_pair(5, COLOR_YELLOW, -1);
  init_pair(6, COLOR_YELLOW, COLOR_WHITE);
  init_pair(7, COLOR_YELLOW, COLOR_RED);

  init_pair(8, COLOR_WHITE, COLOR_YELLOW);
  init_pair(9, COLOR_WHITE, COLOR_WHITE);
  init_pair(10, COLOR_RED, COLOR_RED);
#endif
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
