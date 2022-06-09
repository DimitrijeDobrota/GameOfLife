#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "logic.h"
#include "pattern.h"
#include "utils.h"
#include "window.h"

window_T MAIN_w = NULL;

void cursor_offset(WINDOW *win, int oy, int ox) {
  int y, x;
  getyx(win, y, x);
  wmove(win, y + oy, x + ox);
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

    if (!size) {
      char *message = "NOTHING TO DISPLAY HERE!";
      wcenter_horizontal(wind, y_offset, strlen(message));
      waddstr(win, message);
      wrefresh(win);
      nodelay(stdscr, 0);
      getch();
      nodelay(stdscr, 1);
      return;
    }

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

void handle_winch(int sig) {
  endwin();
  refresh();
  clear();

  window_init(MAIN_w);
  window_update_children(MAIN_w);
}

int display_start(void) {
  curses_start();
  MAIN_w = window_init(window_new());

#ifdef _WIN32
  resize_term(0, 0);
  handle_winch(10);
#endif

  return 1;
}

int display_stop(void) {
  window_free(MAIN_w);
  endwin();
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
