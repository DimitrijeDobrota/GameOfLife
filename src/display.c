/**
 * @file display.c
 * @author Dimitrije Dobrota
 * @date 12 June 2022
 * @brief This file handles ncurses library and UI
 *
 * This file is the main link with ncurses interface. It is responsible for
 * starting and stopping ncurses library as well as managing it's resources and
 * handling terminal resize by rebuilding window_T binary tree. It exports
 * window_T MAIN_w as the entery point for other windows and disiplay calls. It
 * handles the display of menus and user input.
 */
#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "logic.h"
#include "pattern.h"
#include "utils.h"
#include "window.h"

window_T MAIN_w = NULL;
mmask_t  mbitmask;

/**
 * @brief Offset the cursor for ncurses WINDOW*
 *
 * @param oy: real number representing offset along y axis
 * @param ox: real number representing offset along x axis
 */
void cursor_offset(WINDOW *win, int oy, int ox) {
  int y, x;
  getyx(win, y, x);
  wmove(win, y + oy, x + ox);
}

/**
 * @brief Print pattern_T
 *
 * @param y: pointer to int representing y cord where the pattern
 * should be displayed. Will be inc read by the number of lines
 * printed
 * @param x: pointer to int representing x cord where the pattern
 * should be displayed
 */
void print_pattern(WINDOW *win, pattern_T pattern, int *y, int x) {
  (*y)++;
  wmove(win, (*y)++, x);
  for (char *c = pattern->cells; *c != '\0'; c++) {
    if (*c == ' ') {
      wmove(win, (*y)++, x);
      continue;
    }
    int val = *c - '0';
    wattrset(win, COLOR_PAIR(val + 2));
    print_cell(win, "  ");
  }
}

/**
 * @brief Get user input of different type up to the specified size
 *
 * @param buffer: buffer where the input will be stored
 * @param size: maximum number of characters in the input
 * @param crit: function that checks the validity of the character
 */
int input(WINDOW *win, char *buffer, int size, input_f crit) {
  int CLINES = LINES, CCOLS = COLS;
  int ch, read = strlen(buffer);

  while ((ch = getch()) != '\n') {
    switch (ch) {
    case 27:
      flushinp();
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

/**
 * @brief Given a array of struct imenu_T, display a menu where user will enter
 * all of the information required from the array
 */
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

/**
 * @brief Display the title of the game in the center of the screen if it can
 * fit
 */
void display_title(window_T wind, int y) {
  WINDOW *win = window_win(wind);
  title.height = (!title.height) ? pattern_height(&title) : title.height;
  title.width = (!title.width) ? pattern_width(&title) : title.width;

  int max_w = window_wight(wind);
  if (title.width * 2 < max_w)
    print_pattern(win, &title, &y, (max_w - title.width * 2) / 2);
  wrefresh(win);
}

/**
 * @brief Given a array of struct menu_T, display all menu items and allow user
 * to chose one of them after which appropriate callback function will be called
 *
 * @param title: 1 if title should be displayed
 */
void display_menu(window_T wind, char *name, struct menu_T *items, int size,
                  int title) {
  WINDOW *win;
  int     current = 0;

  char sep[] = ">--------<";
  int  sep_len = strlen(sep);

  int maxi = 0, len = 0;
  for (int i = 0; i < size; i++)
    if ((len = strlen(items[i].name)) > maxi)
      maxi = len;

  window_set_title(wind, name);
  window_clear_noRefresh(wind);

  int d_start, d_size, y_offset;

  d_start = 0;
redraw:;
  int CLINES = LINES, CCOLS = COLS;
  win = window_win(wind);

  d_size = window_height(wind) / 2 - 2;

  CLAMP(d_size, 0, size);
  y_offset = wcenter_vertical(wind, d_size * 2) + 1;

  while (TRUE) {
    window_clear(wind);
    if (title)
      display_title(wind, title);

    if (current == -1)
      d_start--;

    if (current == d_size)
      d_start++;

    CLAMP(d_start, 0, size - d_size);
    CLAMP(current, 0, d_size - 1);

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

    if (d_size < size && d_start == 0) {
      wattrset(win, COLOR_PAIR(0));
      wcenter_horizontal(wind, y_offset - 1, sep_len);
      wprintw(win, "%s", sep);
    }

    int index = d_start;
    for (int i = 0; i < d_size; i++, index++) {
      wattrset(win, COLOR_PAIR(i == current ? 1 : 0));
      wcenter_horizontal(wind, y_offset + i * 2, maxi);
      wprintw(win, "%s", items[index].name);
    }

    if (d_size < size && d_start == size - d_size) {
      wattrset(win, COLOR_PAIR(0));
      wcenter_horizontal(wind, y_offset + d_size * 2 - 1, sep_len);
      wprintw(win, "%s", sep);
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
        int index = d_start + current;
        wattrset(win, COLOR_PAIR(0));
        items[index].callback(items[index].name, index);
        return;
      } else if (c == 27) {
        flushinp();
        return;
      }
      if (is_term_resized(CLINES, CCOLS)) {
        HANDLE_RESIZE;
        goto redraw;
      }
    }
  }
}

/**
 * @brief Initialize ncurses library and set colors based on display mode
 * selected while compiling
 */
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

#if defined NCURSES_MOUSE_VERSION && !defined NO_MOUSE
  mbitmask = mousemask(BUTTON1_CLICKED, NULL);
#endif
}

/**
 * @brief Handle the reboiling of window_T binary tree after terminal resize
 *
 * This function MUST be called after a resize has been detected by any function
 */
void handle_winch(int sig) {
  endwin();
  refresh();
  clear();

  window_init(MAIN_w);
  window_update_children(MAIN_w);
}

/**
 * @brief Start ncurses display and export MAIN_w
 */
int display_start(void) {
  curses_start();
  MAIN_w = window_init(window_new());

#ifdef _WIN32
  resize_term(0, 0);
  handle_winch(10);
#endif

  return 1;
}

/**
 * @brief Stop ncurses display and cleanup
 */
int display_stop(void) {
  window_free(MAIN_w);
  endwin();
  return 1;
}

/**
 * @brief Display help menu using all the patterns from all the groups in
 * pattern_groups array
 */
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
        print_pattern(win, &p, &y, x + indent);
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
      flushinp();
      goto end;
    }
    if (is_term_resized(CLINES, CCOLS)) {
      HANDLE_RESIZE;
      goto redraw;
    }
  }
end:;
}
