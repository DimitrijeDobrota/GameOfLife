#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "logic.h"
#include "pattern.h"
#include "utils.h"
#include "window.h"

#define center_vertical(n)      wcenter_vertical(MAIN_W, n);
#define center_horizontal(y, n) wcenter_horizontal(MAIN_W, y, n);

#define CHAR_BLANK  "  "
#define CHAR_CURSOR "<>"
#define CHAR_CIRCLE "\u26AB"
#define CHAR_SQUARE "\u2B1B"

#define CHAR_ACTIVE CHAR_CIRCLE

extern Cell *hash;

window_T MAIN_w = NULL;

#define y_at(y) y, screen_offset_y, h
#define x_at(x) x, screen_offset_x, w

#define for_each_cell(start_i, end_i, start_j, end_j)                          \
  for (int i = start_i; i < end_i; i++)                                        \
    for (int j = start_j; j < end_j; j++)

#ifndef NO_UNICODE
#define print_cell(win, blank)                                                 \
  if (val)                                                                     \
    waddstr(win, CHAR_ACTIVE);                                                 \
  else                                                                         \
    waddstr(win, blank);
#else
#define print_cell(win, blank) waddstr(win, blank);
#endif

// expects val to to be set with value at cordinates
#define mvprint_cell(win, i, j, color_offset, blank)                           \
  {                                                                            \
    wmove(win, i + 1, j * 2 + 1);                                              \
    wattrset(win, COLOR_PAIR(val + color_offset));                             \
    print_cell(win, blank);                                                    \
  }

#define print_cells(win, start_i, end_i, start_j, end_j, color_offset, blank)  \
  for (int i = start_i; i < end_i; i++) {                                      \
    wmove(win, i + 1, 1 + start_j * 2);                                        \
    for (int j = start_j; j < end_j; j++) {                                    \
      int val = getAt(cord(y_at(i)), cord(x_at(j)));                           \
      wattrset(win, COLOR_PAIR(val + color_offset));                           \
      print_cell(win, blank);                                                  \
    }                                                                          \
  }

void print_pattern(WINDOW *win, pattern_T pattern, int *y, int x, int indent) {
  (*y)++;
  wmove(win, (*y)++, x + indent);
  for (char *c = pattern->cells; *c != '\0'; c++) {
    if (*c == ' ') {
      wmove(win, (*y)++, x + indent);
      continue;
    }
    int val = *c - '0';
    wattrset(win, COLOR_PAIR(val + 2));
    print_cell(win, "  ");
  }
}

int screen_offset_x, screen_offset_y;
int cursor_offset_x, cursor_offset_y;

int wrap;

int (*cord)(int, int, int);

int get_screen_position(int value, int screen_offset, int screen_size,
                        int board_size) {
  int overshoot = screen_offset + screen_size - board_size;

  if (wrap) {
    if (overshoot > 0) {
      if (value < screen_offset && value >= overshoot)
        return -1;
      if (value >= screen_offset) {
        return value - screen_offset;
      } else {
        return value + screen_size - overshoot;
      }
    } else {
      if (value < screen_offset || value >= screen_offset + screen_size)
        return -2;
      return value - screen_offset;
    }
  } else {
    if (value < screen_offset || value >= screen_offset + screen_size)
      return -3;
    return value - screen_offset;
  }
}

void display_game(window_T wind, int h, int w, int ph, int pw) {
  WINDOW *win = window_win(wind);

  window_clear_noRefresh(wind);

  int row, col, val;
  for (Cell *c = hash; c != NULL; c = c->hh.next) {
    wattrset(win, COLOR_PAIR(val + 2));

    /* row = get_screen_position(c->row, screen_offset_y, ph, h); */
    /* col = get_screen_position(c->col, screen_offset_x, pw, w); */
    row = get_screen_position(c->cord.row, screen_offset_y, ph, h);
    col = get_screen_position(c->cord.col, screen_offset_x, pw, w);
    val = c->val;

    if (row < 0 || col < 0)
      continue;

    mvprint_cell(win, row, col, 2, CHAR_BLANK);
  }
}

void display_cursor(WINDOW *win, int h, int w, int ph, int pw) {
  static int prev_x = 0, prev_y = 0;
  int        val;

  val = getAt(cord(y_at(prev_y)), cord(x_at(prev_x)));
  mvprint_cell(win, prev_y, prev_x, 2, CHAR_BLANK);

  val = getAt(cord(y_at(cursor_offset_y)), cord(x_at(cursor_offset_x)));
  mvprint_cell(win, cursor_offset_y, cursor_offset_x, 5, CHAR_CURSOR);

  prev_y = cursor_offset_y;
  prev_x = cursor_offset_x;
}

int display_select(window_T wind, int w, int h) {
  int current_offset_y = cursor_offset_y;
  int current_offset_x = cursor_offset_x;
  int ret_value = 1;

  int     CLINES = LINES, CCOLS = COLS;
  WINDOW *win = window_win(wind);
  WINDOW *new;

  if (UNICODE) {
    new = WINDOW_new(wind);
    overlay(win, new);
    wrefresh(new);
  } else {
    new = win;
  }

  int ph = window_height(wind), pw = window_wight(wind) / 2;
  nodelay(stdscr, 0);
  while (TRUE) {
    int start_i = MIN(cursor_offset_y, current_offset_y);
    int end_i = MAX(cursor_offset_y, current_offset_y);
    int start_j = MIN(cursor_offset_x, current_offset_x);
    int end_j = MAX(cursor_offset_x, current_offset_x);

    if (!UNICODE)
      display_game(wind, h, w, ph, pw);

    print_cells(new, start_i, end_i + 1, start_j, end_j + 1, 8, CHAR_BLANK);
    wrefresh(new);

    if (is_term_resized(CLINES, CCOLS)) {
      HANDLE_RESIZE;
      goto end;
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

    // delete selection
    case 'x':
    case 'X':
      for_each_cell(start_i, end_i + 1, start_j, end_j + 1)
          deleteAt(cord(y_at(i)), cord(x_at(j)));
      goto end;

    // toggle selection
    case 't':
    case 'T':
      for_each_cell(start_i, end_i + 1, start_j, end_j + 1)
          toggleAt(cord(y_at(i)), cord(x_at(j)));
      goto end;

    // confirm and save slection
    case '\n':
      for_each_cell(start_i, end_i + 1, start_j, end_j + 1)
          saveCell(cord(y_at(i)), cord(x_at(j)));
      ret_value = 100;
      goto end;

    // quit
    case 27:
    case 'q':
    case 'Q':
      goto end;

    defalut:
      flushinp();
      continue;
    }
    flushinp();

    CLAMP(current_offset_y, 0, ph - 1);
    CLAMP(current_offset_x, 0, pw - 1);

    wclear(new);
    overlay(win, new);
  }
end:;
  nodelay(stdscr, 1);
  delwin(new);
  return ret_value;
}

void display_status(window_T wind, unsigned long int gen, int gen_step,
                    int wrap, int height, int wight, int play, int dt,
                    int cursor_y, int cursor_x) {
  WINDOW *win = window_win(wind);

  wmove(win, 1, 1);
  wprintw(win, " | %5s | ", play ? "play" : "pause");
  wprintw(win, wrap ? "Size: %dx%d | " : "Size: unlimited | ", height, wight);
  wprintw(win, "Generation: %10lu(+%d) | ", gen, gen_step);
  wprintw(win, "dt: %4dms | ", dt);
  wprintw(win, "Cursor: %4dx%-4d | ", cursor_y, cursor_x);
  wrefresh(win);
}

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
  window_clear_noRefresh(wind);
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

void display_title(window_T wind, int y) {
  WINDOW *win = window_win(wind);
  title.height = (!title.height) ? pattern_height(&title) : title.height;
  title.width = (!title.width) ? pattern_width(&title) : title.width;

  int max_w = window_wight(wind);
  if (title.width * 2 < max_w)
    print_pattern(win, &title, &y, (max_w - title.width * 2) / 2, 0);
  wrefresh(win);
}

void display_menu(window_T wind, char *name, struct menu_T *items, int size,
                  int title) {
  WINDOW *win;
  int     current = 0;

  int maxi = 0, len = 0;
  for (int i = 0; i < size; i++)
    if ((len = strlen(items[i].name)) > maxi)
      maxi = len;

  window_set_title(wind, name);
  window_clear_noRefresh(wind);

redraw:;
  win = window_win(wind);
  int CLINES = LINES, CCOLS = COLS;
  int y_offset = wcenter_vertical(wind, size * 2 - 1);

  if (title)
    display_title(wind, title);

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

  init_pair(5, COLOR_WHITE, COLOR_BLUE);
  init_pair(6, COLOR_WHITE, COLOR_BLUE);
  init_pair(7, COLOR_RED, COLOR_BLUE);

  init_pair(8, COLOR_WHITE, COLOR_YELLOW);
  init_pair(9, COLOR_WHITE, COLOR_YELLOW);
  init_pair(10, COLOR_RED, COLOR_YELLOW);
#else
  init_pair(2, COLOR_WHITE, -1);
  init_pair(3, COLOR_WHITE, COLOR_WHITE);
  init_pair(4, COLOR_RED, COLOR_RED);

  init_pair(5, COLOR_BLUE, -1);
  init_pair(6, COLOR_BLUE, COLOR_WHITE);
  init_pair(7, COLOR_BLUE, COLOR_RED);

  init_pair(8, COLOR_WHITE, COLOR_YELLOW);
  init_pair(9, COLOR_WHITE, COLOR_BLUE);
  init_pair(10, COLOR_RED, COLOR_BLACK);
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

void display_patterns(window_T wind) {
  int y_start = 2, x_start = 2;
  int indent = 2;

  window_set_title(wind, "Help");
  window_clear_noRefresh(wind);
redraw:;
  int     CLINES = LINES, CCOLS = COLS;
  WINDOW *win = window_win(wind);
  int     ph = window_height(wind), pw = window_wight(wind);
  int     y, x, maxi, max_x;
  int     count;

  x = x_start;
  for (int i = 0; i < pattern_groups_s; i++) {
    wattrset(win, 0);
    pattern_group_T group = pattern_groups[i];
    y = y_start;

    if ((maxi = strlen(group.name)) + x >= pw)
      continue;
    mvwprintw(win, y++, x, "%s:", group.name);

    y++;
    for (int j = 0; j < group.size; j++) {
      struct pattern_T p = group.pattern[j];
      p.height = (!p.height) ? pattern_height(&p) : p.height;
      p.width = (!p.width) ? pattern_width(&p) : p.width;

      max_x = MAX(p.width * 2, strlen(p.name) + 3);

      if (y + p.height + 4 >= ph) {
        y = 3;
        x += (maxi + 2 + indent * 1);
        maxi = max_x;
      }

      if (x + max_x + 5 + indent >= pw) {
        break;
      }

      wattrset(win, 0);
      if (p.height != 0) {
        mvwprintw(win, y++, x, "- %s: ", p.name);
        print_pattern(win, &p, &y, x, indent);
        y += 2;
      } else {
        if (*p.name) {
          if (*p.cells != '\0') {
            mvwprintw(win, y, x, "  %s: ", p.name);
            max_x += 2;
          } else
            mvwprintw(win, y, x, "%s: ", p.name);
          wprintw(win, "%s", p.cells);
        }
        y++;
      }

      maxi = MAX(maxi, max_x);
    }
    x += (maxi + 2 + indent * 2);

    wattrset(win, 0);
    for (int i = 1; i <= ph; i++)
      mvwprintw(win, i, x, "|");

    x += 2 * indent;
    if (x >= pw)
      break;
  }

  wrefresh(win);
  while (true) {
    switch (getch()) {
    case 27:
    case 'q':
    case 'Q':
    case '\n':
      goto end;
    }
    if (is_term_resized(CLINES, CCOLS)) {
      HANDLE_RESIZE;
      goto redraw;
    }
  }
end:;
}
