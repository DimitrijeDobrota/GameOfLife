/**
 * @file game.c
 * @author Dimitrije Dobrota
 * @date 16 June 2022
 * @brief This file handles displaying and managing game using logic module
 *
 * This file contain function for both managing and displaying the game
 * state. Apart from the main game runner that relies on logic module there are
 * function for displaying game, cursor, and status. It takes care of screen and
 * cursor position as well as handle mouse input.
 */

#include <curses.h>
#include <time.h>

#include "display.h"
#include "game.h"
#include "logic.h"
#include "main.h"
#include "utils.h"
#include "window.h"

#define DEF_GEN_STEP    1
#define DEF_SCREEN_STEP 1
#define DEF_TIME_CONST  100
#define DEF_TIME_STEP   1

#ifdef _WIN32
#define TIME_MOD 1
#else
#define TIME_MOD 1000
#endif

extern char *evolution_names[];
extern int   evolution_cells[];
extern int   evolution_size;

extern window_T menu_w;
extern mmask_t  mbitmask;
extern Cell    *hash;

typedef int (*coordinate_f)(int, int, int);

/**
 * @brief Given a coordinate relative to the screen, return real game coordinate
 * if not wrapping
 */
int coordinate_nowrap(int val, int offset, int max) { return val + offset; }

/**
 * Given a coordinate relative to the screen, return real game coordinate
 * wrapping
 */
int coordinate_wrap(int val, int offset, int max) {
  return (val + offset + max) % max;
}

coordinate_f
    cord; ///< function to be used to get real coordinates from relative ones

int height, width;

static int win_height, win_width;
static int screen_offset_x, screen_offset_y;
static int cursor_offset_x, cursor_offset_y;
static int wrap, gen_step, screen_step;
static int play, time_const, time_step;

static unsigned gen;

#define y_at(y) y, screen_offset_y, height
#define x_at(x) x, screen_offset_x, width

/**
 * @brief Convenience macro for looping over all the cells in a given coordinate
 * range
 */
#define for_each_cell(start_i, end_i, start_j, end_j)                          \
  for (int i = start_i; i < end_i; i++)                                        \
    for (int j = start_j; j < end_j; j++)

/**
 * @brief Display cell at given game coordinates  to the ncurses WINDOW.
 * Requires int val to be set to the value of the cell to be displayed
 */
#define mvprint_cell(win, i, j, color_offset, blank)                           \
  {                                                                            \
    wmove(win, i + 1, j * 2 + 1);                                              \
    wattrset(win, COLOR_PAIR(val + color_offset));                             \
    print_cell(win, blank);                                                    \
  }

/**
 * @brief Given a coordinate range, display that part of the game to the ncurses
 * WINDOW. Use color scheme defined in color_offset, with dead cells being
 * filled with blank
 */
#define print_cells(win, start_i, end_i, start_j, end_j, color_offset, blank)  \
  for (int i = start_i; i < end_i; i++) {                                      \
    wmove(win, i + 1, 1 + start_j * 2);                                        \
    for (int j = start_j; j < end_j; j++) {                                    \
      int val = getAt(cord(y_at(i)), cord(x_at(j)));                           \
      wattrset(win, COLOR_PAIR(val + color_offset));                           \
      print_cell(win, blank);                                                  \
    }                                                                          \
  }

/**
 * @brief Given a coordinate, screen offset, screen size, and board size
 * calculate the exact position on the screen or return a negative number if
 * it shouldn't be displayed.
 *
 * @param coordinate: x or y coordinates of a point to be checked
 * @param screen_offset: screen offset along the x or y axis
 * @param screen_size: screen size on the x or y axis
 * @param board_size: game size of x or y axis, needed if wrapping
 */
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

/**
 * @brief Display the part of the game seen by screen to the ncurses WINDOW
 * provided
 */
void display_game(window_T wind) {
  WINDOW *win = window_win(wind);

  window_clear_noRefresh(wind);

  int row, col, val;
  for (Cell *c = hash; c != NULL; c = c->hh.next) {
    wattrset(win, COLOR_PAIR(val + 2));

    row = get_screen_position(c->cord.row, screen_offset_y, win_height, height);
    col = get_screen_position(c->cord.col, screen_offset_x, win_width, width);
    val = c->val;

    if (row < 0 || col < 0)
      continue;

    mvprint_cell(win, row, col, 2, CHAR_BLANK);
  }
}

/**
 * @brief Display the cursor, fixing the previous position, to the ncurses
 * WINDOW provided
 */
void display_cursor(WINDOW *win) {
  static int prev_x = 0, prev_y = 0;
  int        val;

  val = getAt(cord(y_at(prev_y)), cord(x_at(prev_x)));
  mvprint_cell(win, prev_y, prev_x, 2, CHAR_BLANK);

  val = getAt(cord(y_at(cursor_offset_y)), cord(x_at(cursor_offset_x)));
  mvprint_cell(win, cursor_offset_y, cursor_offset_x, 5, CHAR_CURSOR);

  prev_y = cursor_offset_y;
  prev_x = cursor_offset_x;
}

/**
 * @brief Display game information to the ncurses WINDOW provided
 */
void display_status(window_T wind) {
  WINDOW *win = window_win(wind);

  wmove(win, 1, 1);
  wprintw(win, " %5s | ", play ? "play" : "pause");
  wprintw(win, wrap ? "Size: %9dx%9d | " : "Size: unlimited | ", height, width);
  wprintw(win, "Generation: %10u(+%d) | ", gen, gen_step);
  wprintw(win, "dt: %4dms | ", time_const);
  wprintw(win, "Cursor: %10dx%10d | ", cord(y_at(cursor_offset_y)),
          cord(x_at(cursor_offset_x)));
  wrefresh(win);
}

/**
 * @brief Display the visual select mode
 *
 * This function is interactive:
 * - Use the wasd to move the selection around
 * - Use x to delete all selected cells
 * - Use t to toggle all selected cells
 * - Use enter to save the selected cells as a pattern
 * - Use q or esc to quit visual select mode
 */
int display_select(window_T wind) {
  int CLINES = LINES, CCOLS = COLS;

  int current_offset_y = cursor_offset_y;
  int current_offset_x = cursor_offset_x;
  int ret_value = 1;

  WINDOW *win = window_win(wind);
  WINDOW *new;

  if (UNICODE) {
    new = window_win_new(wind);
    overlay(win, new);
    wrefresh(new);
  } else {
    new = win;
  }

  int ph = window_height(wind), pw = window_width(wind) / 2;
  nodelay(stdscr, 0);
  while (TRUE) {
    int start_i = MIN(cursor_offset_y, current_offset_y);
    int end_i = MAX(cursor_offset_y, current_offset_y);
    int start_j = MIN(cursor_offset_x, current_offset_x);
    int end_j = MAX(cursor_offset_x, current_offset_x);

    if (!UNICODE)
      display_game(wind);

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
    case KEY_UP:
      current_offset_y--;
      break;
    case 's':
    case 'S':
    case KEY_DOWN:
      current_offset_y++;
      break;
    case 'a':
    case 'A':
    case KEY_LEFT:
      current_offset_x--;
      break;
    case 'd':
    case 'D':
    case KEY_RIGHT:
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

    // confirm and save selection
    case '\n':
      for_each_cell(start_i, end_i + 1, start_j, end_j + 1)
          saveCell(cord(y_at(i)), cord(x_at(j)));
      ret_value = 100;
      goto end;

    // quit
    case 27:
    case 'q':
    case 'Q':
      flushinp();
      ret_value = 1;
      goto end;
    }
    flushinp();

    CLAMP(current_offset_y, 0, ph - 1);
    CLAMP(current_offset_x, 0, pw - 1);

    wclear(new);
    overlay(win, new);
  }
end:;

  if (UNICODE)
    delwin(new);

  nodelay(stdscr, 1);
  return ret_value;
}

/**
 * @brief save the current screen and cursor coordinates
 * before resize to use them after redraw. Must be used before goto redraw
 */
#define save_state()                                                           \
  {                                                                            \
    t_y = cord(y_at(win_height / 2));                                          \
    t_x = cord(x_at(win_width / 2));                                           \
    ct_y = cord(y_at(cursor_offset_y));                                        \
    ct_x = cord(x_at(cursor_offset_x));                                        \
  }

/**
 * @brief Main game runner. Connection between logic and display
 * This function is responsive:
 * - On every resize:
 *   - window tree is recreated
 *   - new screen size is calculated with fixed center
 *   - cursor is clipped to the screen
 *   - game is redrawn
 *
 * This function is interactive:
 * - Use p to play/pause the simulation
 * - Use the arrow keys to move the screen around
 * - Use -/+ to decrease or increase the numbs of evolutions before displaying
 * change
 * - Use [/] to decrease or increase time wait before update
 * - Use q or esc to return to the main menu
 * - If not play:
 *   - Use wasd to move the cursor around
 *   - Use space or mouse click to toggle cell state
 *   - Use v to enter the visual select mode
 *   - Use l to load the pattern
 *   - Use o to save the current game
 *   - Use h to show help menu
 *   - Use r to redraw the screen
 */
void game(int s_h, int s_w, int mode_index) {
  char *mode_name = evolution_names[mode_index];

  int t_y = 0, t_x = 0, ct_x = 0, ct_y = 0;
  wrap = 1;

  window_T status_w, screen_w, game_w;

  gen = 0;
  gen_step = DEF_GEN_STEP, time_const = DEF_TIME_CONST;
  time_step = DEF_TIME_STEP, screen_step = DEF_SCREEN_STEP;

reset_screen:
  status_w = window_split(menu_w, 1, 3, 0, "Status", "Game");
  screen_w = window_sibiling(status_w);
  window_set_title(menu_w, NULL);
  window_clear(menu_w);

  wrap = (s_w > 0 && s_h > 0);

  if (wrap) {
    width = s_w;
    height = s_h;
  } else {
    width = 0;
    height = 0;
  }

  logic_init(wrap, mode_index);

  cord = wrap ? coordinate_wrap : coordinate_nowrap;
  game_w = window_center(screen_w, height, width * 2, mode_name);

redraw:;
  int     CLINES = LINES, CCOLS = COLS;
  clock_t start_t, end_t = 0, total_t;
  MEVENT  mort;

  if (!wrap) {
    game_w = window_center(screen_w, window_height(screen_w),
                           window_width(screen_w), mode_name);
  }

  WINDOW *game_W = window_win(game_w);
  win_height = window_height(game_w), win_width = window_width(game_w) / 2;

  window_clear(menu_w);
  window_clear(screen_w);
  window_clear(status_w);

  screen_offset_y = t_y - win_height / 2;
  screen_offset_x = t_x - win_width / 2;

  if (wrap) {
    screen_offset_x = (screen_offset_x + width) % width;
    screen_offset_y = (screen_offset_y + height) % height;
  }

  cursor_offset_y = ct_y - screen_offset_y;
  cursor_offset_x = ct_x - screen_offset_x;

  if (wrap) {
    cursor_offset_x = (cursor_offset_x + width) % width;
    cursor_offset_y = (cursor_offset_y + height) % height;
  }

  CLAMP(cursor_offset_y, 0, win_height - 1);
  CLAMP(cursor_offset_x, 0, win_width - 1);

  display_game(game_w);
  display_cursor(game_W);
  wrefresh(game_W);

  int screen_change = 1;
  int cursor_change = 1;

  while (TRUE) {
    start_t = clock();

    if (wrap) {
      screen_offset_x = (screen_offset_x + width) % width;
      screen_offset_y = (screen_offset_y + height) % height;
    }

    if (play) {
      do_evolution(gen_step);
      screen_change = 1;
      gen += gen_step;
    }

    display_status(status_w);

    if (screen_change) {
      display_game(game_w);
      screen_change = 0;
      cursor_change = 1;
    }

    if (cursor_change) {
      display_cursor(game_W);
      wrefresh(game_W);
      cursor_change = 0;
    }

    while ((total_t = (long int)(end_t - start_t)) < time_const * TIME_MOD) {
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
        flushinp();
        goto end;

      // change num of evolutions before display
      case '+':
        gen_step++;
        break;
      case '-':
        gen_step--;
        break;

      // change refresh rate
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

#if defined NCURSES_MOUSE_VERSION && !defined NO_MOUSE
        // mouse  toggle cell
        case KEY_MOUSE:
          getmouse(&mort);
          if (!window_clicked(game_w, mort.y, mort.x))
            break;

          int mouse_offset_y = mort.y - window_y(game_w) - 1;
          int mouse_offset_x = (mort.x - window_x(game_w) - 1) / 2;
          int val =
              toggleAt(cord(y_at(mouse_offset_y)), cord(x_at(mouse_offset_x)));

          if (mouse_offset_x != cursor_offset_x ||
              mouse_offset_y != cursor_offset_y) {
            mvprint_cell(game_W, mouse_offset_y, mouse_offset_x, 2, CHAR_BLANK);
            wrefresh(game_W);
          } else
            cursor_change = 1;
          break;
#endif

        // toggle cell
        case ' ':
          toggleAt(cord(y_at(cursor_offset_y)), cord(x_at(cursor_offset_x)));
          cursor_change = 1;
          break;

        // visual selection
        case 'v':
        case 'V':
          if (display_select(game_w) == 100) {
            window_unsplit(menu_w);
            save_pattern();
          }

          save_state();
          goto reset_screen;

        // lead pattern
        case 'l':
        case 'L':
          window_unsplit(menu_w);
          setPosition(cord(y_at(cursor_offset_y)), cord(x_at(cursor_offset_x)));
          load_pattern();

          save_state();
          goto reset_screen;

        // save game
        case 'o':
        case 'O':
          window_unsplit(menu_w);
          save();

          save_state();
          goto reset_screen;

        // help menu
        case 'h':
        case 'H':
          window_unsplit(menu_w);
          display_patterns(menu_w);

          save_state();
          goto reset_screen;

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
        screen_offset_x -= screen_step;
        screen_change = 1;
        break;
      case KEY_A3:
      case KEY_C3:
      case KEY_NPAGE:
      case KEY_PPAGE:
      case KEY_RIGHT:
        screen_offset_x += screen_step;
        screen_change = 1;
        break;
      }

      switch (c) {
      case KEY_A1:
      case KEY_A3:
      case KEY_HOME:
      case KEY_PPAGE:
      case KEY_UP:
        screen_offset_y -= screen_step;
        screen_change = 1;
        break;
      case KEY_C1:
      case KEY_C3:
      case KEY_DOWN:
      case KEY_END:
      case KEY_NPAGE:
        screen_offset_y += screen_step;
        screen_change = 1;
      }

      CLAMP(cursor_offset_y, 0, win_height - 1);
      CLAMP(cursor_offset_x, 0, win_width - 1);

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
  }
end:;
  window_unsplit(menu_w);
  logic_free();
  return;
}
