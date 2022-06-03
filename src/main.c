#include <ctype.h>
#include <curses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include <time.h>

#include "display.h"
#include "file.h"
#include "logic.h"
#include "utils.h"
#include "window.h"

#ifdef _WIN32
#define TIME_MOD 1
#else
#define TIME_MOD 1000
#endif

extern window_T MAIN_w;
extern char    *evolution_names[];
extern int      evolution_cells[];
extern int      evolution_size;

window_T menu_w;
int      top_space = 5;

void load_pattern(void);
void save_pattern(void);

#define save_state()                                                           \
  {                                                                            \
    t_y = cord(y_at(ph / 2));                                                  \
    t_x = cord(x_at(pw / 2));                                                  \
    ct_y = cord(y_at(cursor_offset_y));                                        \
    ct_x = cord(x_at(cursor_offset_x));                                        \
  }

#define y_at(y) y, screen_offset_y, h
#define x_at(x) x, screen_offset_x, w

typedef int (*coordinate_f)(int, int, int);
int coordinate_nowrap(int val, int offset, int max) { return val + offset; }
int coordinate_wrap(int val, int offset, int max) {
  return (val + offset + max) % max;
}

extern coordinate_f cord;

extern int screen_offset_x, screen_offset_y;
extern int cursor_offset_x, cursor_offset_y;

extern short int **mat;

void game(int h, int w, char *name, int ncells, int mode_index) {
  unsigned long int gen = 0;
  int               gen_step = 1, play = 0, time_const = 100, time_step = 1;

  int wrap = 1;
  int ph, pw, t_y = 0, t_x = 0, ct_x = 0, ct_y = 0;

  window_T status_w, screen_w, game_w;

  status_w = window_split(menu_w, 1, 3, 0, "Status", name);
  screen_w = window_sibiling(status_w);
  window_set_title(menu_w, NULL);
  window_clear(menu_w);

  wrap = (w > 0 && h > 0);

  if (!wrap) {
    w = window_wight(screen_w) / 2;
    h = window_height(screen_w);
  }

  cord = wrap ? coordinate_wrap : coordinate_nowrap;
  game_w = window_center(screen_w, 0, h, w * 2, "Game");

  logic_init(w, h, wrap);
  evolution_init(mode_index);

redraw:;
  int     CLINES = LINES, CCOLS = COLS;
  clock_t start_t, end_t = 0, total_t;

  if (!wrap) {
    game_w = window_center(screen_w, 0, window_height(screen_w),
                           window_wight(screen_w), "Game");
  }

  WINDOW *game_W = window_win(game_w);
  ph = window_height(game_w), pw = window_wight(game_w) / 2;

  window_clear(menu_w);
  window_clear(screen_w);
  window_clear(status_w);

  screen_offset_y = t_y - ph / 2;
  screen_offset_x = t_x - pw / 2;

  if (wrap) {
    screen_offset_x = (screen_offset_x + w) % w;
    screen_offset_y = (screen_offset_y + h) % h;
  }

  cursor_offset_y = ct_y - screen_offset_y;
  cursor_offset_x = ct_x - screen_offset_x;

  if (wrap) {
    cursor_offset_x = (cursor_offset_x + w) % w;
    cursor_offset_y = (cursor_offset_y + h) % h;
  }

  CLAMP(cursor_offset_y, 0, ph - 1);
  CLAMP(cursor_offset_x, 0, pw - 1);

  display_game(game_W, h, w, ph, pw, wrap);
  display_cursor(game_W, h, w, ph, pw);

  int screen_change = 1;
  int cursor_change = 1;

  while (TRUE) {
    start_t = clock();

    if (wrap) {
      screen_offset_x = (screen_offset_x + w) % w;
      screen_offset_y = (screen_offset_y + h) % h;
    }

    if (play) {
      do_evolution(gen_step);
      screen_change = 1;
      gen += gen_step;
    }

    display_status(status_w, gen, gen_step, wrap, h, w, play, time_const,
                   cord(y_at(cursor_offset_y)), cord(x_at(cursor_offset_x)));

    if (play || screen_change) {
      display_game(game_W, h, w, ph, pw, wrap);
      screen_change = 0;
      cursor_change = 1;
    }

    if (cursor_change) {
      display_cursor(game_W, h, w, ph, pw);
      cursor_change = 0;
    }

    while ((total_t = (long int)(end_t - start_t)) < time_const * TIME_MOD) {
      refresh();
      int c = getch();
      switch (c) {

      // toggle pause
      case 'p':
      case 'P':
        play = !play;
        break;

      // quit
      case 27:
      case 'q':
      case 'Q':
        goto end;

      // change num of evolutions before display
      case '+':
        gen_step++;
        break;
      case '-':
        gen_step--;
        break;

      // change refreshrate
      case ']':
        time_const += time_step;
        break;
      case '[':
        time_const -= time_step;
        break;
      }

      if (!play) {
        // move cursor around
        switch (c) {
        case 'w':
        case 'W':
          cursor_offset_y--;
          cursor_change = 1;
          break;
        case 's':
        case 'S':
          cursor_offset_y++;
          cursor_change = 1;
          break;
        case 'a':
        case 'A':
          cursor_offset_x--;
          cursor_change = 1;
          break;
        case 'd':
        case 'D':
          cursor_offset_x++;
          cursor_change = 1;
          break;

        // toggle cell
        case ' ':
          toggleAt(cord(y_at(cursor_offset_y)), cord(x_at(cursor_offset_x)));
          cursor_change = 1;
          break;

        // visual selection
        case 'v':
        case 'V':
          if (display_select(game_w, h, h) == 100) {
            save_pattern();
          }

          window_set_title(menu_w, NULL);
          save_state();
          goto redraw;
          break;

        // lead pattern
        case 'l':
        case 'L':
          load_pattern();

          window_set_title(screen_w, name);
          window_set_title(menu_w, NULL);
          save_state();
          goto redraw;
          break;

        // redraw screen
        case 'r':
        case 'R':
          save_state();
          goto redraw;
        }
      }

      // move screen around
      switch (c) {
      case KEY_A1:
      case KEY_C1:
      case KEY_END:
      case KEY_HOME:
      case KEY_LEFT:
        screen_offset_x--;
        screen_change = 1;
        break;
      case KEY_A3:
      case KEY_C3:
      case KEY_NPAGE:
      case KEY_PPAGE:
      case KEY_RIGHT:
        screen_offset_x++;
        screen_change = 1;
        break;
      }

      switch (c) {
      case KEY_A1:
      case KEY_A3:
      case KEY_HOME:
      case KEY_PPAGE:
      case KEY_UP:
        screen_offset_y--;
        screen_change = 1;
        break;
      case KEY_C1:
      case KEY_C3:
      case KEY_DOWN:
      case KEY_END:
      case KEY_NPAGE:
        screen_offset_y++;
        screen_change = 1;
      }

      CLAMP(cursor_offset_y, 0, ph - 1);
      CLAMP(cursor_offset_x, 0, pw - 1);

      CLAMP(gen_step, 1, 100);
      CLAMP(time_const, 0, 1000);

      if (is_term_resized(CLINES, CCOLS)) {
        flushinp();
        save_state();
        HANDLE_RESIZE;
        goto redraw;
      }
      end_t = clock();
    }
    flushinp();
  }
end:;
  window_unsplit(menu_w);
  logic_free();
  return;
}

void settings(char *pass, int index) {
  struct imenu_T imenu_items[] = {
      {   "Number of rows", 6, isdigit, NULL},
      {"Number of columns", 6, isdigit, NULL},
  };
  int imenu_items_s = sizeof(imenu_items) / sizeof(struct imenu_T);

  window_set_title(menu_w, "Game settings");
  while (display_imenu(menu_w, imenu_items, imenu_items_s)) {
    int row = atoi(imenu_items[0].buffer);
    int column = atoi(imenu_items[1].buffer);

    game(row, column, pass, evolution_cells[index], index);
    break;
  }

  for (int i = 0; i < imenu_items_s; i++) {
    free(imenu_items[i].buffer);
    imenu_items[i].buffer = NULL;
  }
}

void mode_select(char *pass, int index) {
  struct menu_T *mode_items = malloc(evolution_size * sizeof(struct menu_T));
  for (int i = 0; i < evolution_size; i++) {
    mode_items[i].name = evolution_names[i];
    mode_items[i].callback = settings;
  }
  display_menu(menu_w, "Game Mode", mode_items, evolution_size);

  free(mode_items);
}

void new_file(char *pass, int index) {
  struct imenu_T new_file_items[] = {
      {"Pick a name", 10, isalnum, NULL},
  };
  int new_file_items_s = sizeof(new_file_items) / sizeof(struct imenu_T);

  window_set_title(menu_w, "New File");
  while (display_imenu(menu_w, new_file_items, new_file_items_s)) {
    file_save_pattern(new_file_items[0].buffer, index);
    break;
  }

  for (int i = 0; i < new_file_items_s; i++) {
    free(new_file_items[i].buffer);
    new_file_items[i].buffer = NULL;
  }
}

struct menu_T *file_menu_list(char *ext, void (*callback)(char *, int),
                              int offset, int *size) {
  char **buffer;
  int    n;

  load_files();
  n = file_select("part", &buffer);

  struct menu_T *file_items = malloc((n + offset) * sizeof(struct menu_T));
  for (int i = 0; i < n; i++) {
    file_items[i + offset].name = buffer[i];
    file_items[i + offset].callback = callback;
  }
  free(buffer);

  *size = n + offset;
  return file_items;
}

void save_pattern(void) {
  int            n;
  struct menu_T *file_items = file_menu_list("part", file_save_pattern, 1, &n);
  file_items[0].name = "NEW";
  file_items[0].callback = new_file;

  display_menu(menu_w, "Save Pattern", file_items, n);
  free(file_items);
}

void load_pattern(void) {
  int            n;
  struct menu_T *file_items = file_menu_list("part", file_load_pattern, 0, &n);

  display_menu(menu_w, "Load Pattern", file_items, n);
  free(file_items);
}

void load(char *pass, int index) {
  int            n;
  struct menu_T *file_items = file_menu_list("part", file_load, 0, &n);

  display_menu(menu_w, "Load Game", file_items, n);
  free(file_items);

  mode_select("nothing", 0);
}

void exitp(char *pass, int index) {
  display_stop();
  exit(0);
}

struct menu_T menu_items[] = {
    {mode_select, "Start"},
    {       load,  "Load"},
    {      exitp,  "Exit"}
};

int menu_items_s = sizeof(menu_items) / sizeof(struct menu_T);

int main(void) {
  setlocale(LC_ALL, "");

  if (!file_setup()) {
    printf("File setup error\n");
    abort();
  }

  if (!display_start()) {
    printf("Couldn't start the display!\n");
    abort();
  }

  menu_w = MAIN_w;
  while (TRUE) {
    display_menu(menu_w, "Main menu", menu_items, menu_items_s);
  }

  window_free(MAIN_w);

  if (!display_stop()) {
    printf("Couldn't stop the display!\n");
    abort();
  }

  return 0;
}
