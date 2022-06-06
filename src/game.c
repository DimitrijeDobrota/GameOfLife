#include <curses.h>
#include <time.h>

#include "display.h"
#include "utils.h"
#include "window.h"
#include "logic.h"
#include "main.h"

#ifdef _WIN32
#define TIME_MOD 1
#else
#define TIME_MOD 1000
#endif

extern char *evolution_names[];
extern int   evolution_cells[];
extern int   evolution_size;
extern window_T menu_w;

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
extern int wrap;

void game(int s_h, int s_w, char *mode_name, int ncells, int mode_index) {
  unsigned long int gen = 0;
  int               h, w;
  int               gen_step = 1, play = 0, time_const = 100, time_step = 1;

  wrap = 1;
  int ph, pw, t_y = 0, t_x = 0, ct_x = 0, ct_y = 0;

  window_T status_w, screen_w, game_w;

reset_screen:
  status_w = window_split(menu_w, 1, 3, 0, "Status", "Game");
  screen_w = window_sibiling(status_w);
  window_set_title(menu_w, NULL);
  window_clear(menu_w);

  wrap = (s_w > 0 && s_h > 0);

  if (!wrap) {
    w = window_wight(screen_w) / 2;
    h = window_height(screen_w);
    logic_init(0, 0, wrap);
  } else {
    w = s_w;
    h = s_h;
    logic_init(w, h, wrap);
  }

  cord = wrap ? coordinate_wrap : coordinate_nowrap;
  game_w = window_center(screen_w, 0, h, w * 2, mode_name);

  evolution_init(mode_index);

redraw:;
  int     CLINES = LINES, CCOLS = COLS;
  clock_t start_t, end_t = 0, total_t;

  if (!wrap) {
    game_w = window_center(screen_w, 0, window_height(screen_w),
                           window_wight(screen_w), mode_name);
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

  display_game(game_w, h, w, ph, pw);
  display_cursor(game_W, h, w, ph, pw);
  wrefresh(game_W);

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
      display_game(game_w, h, w, ph, pw);
      wrefresh(game_W);
      screen_change = 0;
      cursor_change = 1;
    }

    if (cursor_change) {
      display_cursor(game_W, h, w, ph, pw);
      wrefresh(game_W);
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
            window_unsplit(menu_w);
            save_pattern();
          }

          save_state();
          goto reset_screen;
          break;

        // lead pattern
        case 'l':
        case 'L':
          window_unsplit(menu_w);
          setPosition(cord(y_at(cursor_offset_y)), cord(x_at(cursor_offset_x)));
          load_pattern();
          save_state();
          goto reset_screen;
          break;

        // save game
        case 'o':
        case 'O':
          window_unsplit(menu_w);
          save();
          save_state();
          goto reset_screen;
          break;

        // help menu
        case 'h':
        case 'H':
          window_unsplit(menu_w);
          display_patterns(menu_w);
          save_state();
          goto reset_screen;
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
