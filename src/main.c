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
#define TIME_CONST 100
#else
#define TIME_CONST 100000
#endif

extern window_T MAIN_w;
extern cell   **mat;
extern char    *evolution_names[];
extern int      evolution_cells[];
extern int      evolution_size;

window_T status_w, screen_w, game_w;
int      top_space = 5;

void game(window_T wind, int w, int h, int ncells) {
  int screen_offset_y = 0, screen_offset_x = 0;
  int cursor_offset_y = 0, cursor_offset_x = 0;
  int in = 3;
  int gen_step = 1;
  int play = 0;
  int gen = 1;

  window_clear(wind);
  game_w = window_center(wind, 0, h, w * 2);

  clock_t start_t, end_t = 0, total_t;
redraw:;
  int CLINES = LINES, CCOLS = COLS;

  while (TRUE) {
    start_t = clock();
    display_game(game_w, mat, w, h, screen_offset_y, screen_offset_x,
                 &cursor_offset_y, &cursor_offset_x);
    if (play) {
      do_evolution(gen_step);
      gen++;
    }

    while ((total_t = (long int)(end_t - start_t)) < TIME_CONST) {
      refresh();
      int c = getch();
      switch (c) {
      case 'p':
      case 'P':
        play = !play;
        break;
      case 27:
        return;
      case '+':
        gen_step++;
        break;
      case '-':
        gen_step--;
        break;
      case KEY_A1:
      case KEY_C1:
      case KEY_END:
      case KEY_HOME:
      case KEY_LEFT:
        screen_offset_x--;
        break;
      case KEY_A3:
      case KEY_C3:
      case KEY_NPAGE:
      case KEY_PPAGE:
      case KEY_RIGHT:
        screen_offset_x++;
        break;
      }

      switch (c) {
      case KEY_A1:
      case KEY_A3:
      case KEY_HOME:
      case KEY_PPAGE:
      case KEY_UP:
        screen_offset_y--;
        break;
      case KEY_C1:
      case KEY_C3:
      case KEY_DOWN:
      case KEY_END:
      case KEY_NPAGE:
        screen_offset_y++;
      }

      if (!play) {
        switch (c) {
        case 'w':
        case 'W':
          cursor_offset_y--;
          break;
        case 's':
        case 'S':
          cursor_offset_y++;
          break;
        case 'a':
        case 'A':
          cursor_offset_x--;
          break;
        case 'd':
        case 'D':
          cursor_offset_x++;
          break;
        case ' ':
          mat[(cursor_offset_y + screen_offset_y + h) % h + 1]
             [(cursor_offset_x + screen_offset_x + w) % w + 1] =
                 (mat[(cursor_offset_y + screen_offset_y + h) % h + 1]
                     [(cursor_offset_x + screen_offset_x + w) % w + 1] +
                  1) %
                 ncells;
          break;
        }
      }
      screen_offset_x = (screen_offset_x + w) % w;
      screen_offset_y = (screen_offset_y + h) % h;
      CLAMP(gen_step, 1, 10);

      /* usleep(100000); */
      if (is_term_resized(CLINES, CCOLS)) {
        flushinp();
        HANDLE_RESIZE;
        goto redraw;
      }
      end_t = clock(); // clock stopped
    }
    flushinp();
  }
}

struct imenu_T imenu_items[] = {
    {"Unesi broj redova", 4, isdigit, NULL},
    {"Unesi broj kolona", 4, isdigit, NULL},
};
int imenu_items_s = sizeof(imenu_items) / sizeof(struct imenu_T);

void settings(window_T wind, char *pass, int index) {
  while (TRUE) {
    if (display_imenu(wind, imenu_items, imenu_items_s)) {
      int row = atoi(imenu_items[0].buffer);
      int column = atoi(imenu_items[1].buffer);

      if (!row || !column)
        continue;

      logic_init(column, row);
      evolution_init(index);

      game(wind, row, column, evolution_cells[index]);
      break;
    } else {
      break;
    }
  }

  for (int i = 0; i < imenu_items_s; i++) {
    free(imenu_items[i].buffer);
    imenu_items[i].buffer = NULL;
  }
}

void mode_select(window_T wind, char *pass, int index) {
  struct menu_T *mode_items = malloc(evolution_size * sizeof(struct menu_T));
  for (int i = 0; i < evolution_size; i++) {
    mode_items[i].name = evolution_names[i];
    mode_items[i].callback = settings;
  }
  display_menu(wind, mode_items, evolution_size);

  free(mode_items);
}

void load(window_T wind, char *pass, int index) {
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
  mode_select(wind, "nothing", 0);

  free(file_items);
  free_files();
}

void exitp(window_T wind, char *pass, int index) {
  display_stop();
  exit(0);
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

  status_w = window_split(MAIN_w, 1, 3, 0);
  screen_w = window_sibiling(status_w);

  while (TRUE) {
    display_menu(screen_w, menu_items, menu_items_s);
    window_clear(screen_w);
  }

  if (!display_stop()) {
    printf("Couldn't stop the display!\n");
  }

  return 0;
}
