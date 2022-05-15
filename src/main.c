// This is not a final version of the code
// This is just a quick proof of concept
// Many things need to be addressed later

#include <ctype.h>
#include <curses.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

WINDOW *game_w, *board_w, *status_w;
int     top_space = 5;

typedef struct file_T *file_T;
struct file_T {
  file_T next;
  char  *name;
};

file_T file_new(char *name) {
  file_T f;
  f = malloc(sizeof(*f));
  f->next = NULL;
  if (name == NULL) {
    f->name = NULL;
    return f;
  }

  f->name = malloc((strlen(name) + 1) * sizeof(char));
  strcpy(f->name, name);
  return f;
}

void file_free(file_T self) {
  if (self == NULL)
    return;
  file_free(self->next);
  free(self->name);
  free(self);
}

file_T file_add(file_T self, char *name) {
  for (; self->next != NULL; self = self->next)
    ;
  self->next = file_new(name);
  return self->next;
}

void file_list(file_T self) {
  for (; self != NULL; self = self->next)
    printw("%s ", self->name);
}

int DirectoryExists(const char *path) {
  struct stat stats;
  stat(path, &stats);

  return S_ISDIR(stats.st_mode);
}

file_T file_fromDirectory(void) {
  file_T base = file_new(NULL);

  struct dirent *de;
  DIR           *dr = opendir(".");
  if (dr == NULL)
    return NULL;

  move(0, 0);
  while ((de = readdir(dr)) != NULL)
    file_add(base, de->d_name);

  file_T n = base->next;

  free(base);
  free(dr);

  return n;
}

file_T file_find(file_T self, char *name) {
  for (; self != NULL; self = self->next) {
    if (!strcmp(self->name, name))
      return self;
  }
  return NULL;
}

struct extension_T {
  char  *ext;
  file_T list;
};

void file_filter(file_T self, struct extension_T *ext, int ext_s) {
  file_T *tmp = malloc(ext_s * sizeof(file_T));
  for (int i = 0; i < ext_s; i++) {
    tmp[i] = ext[i].list = file_new(NULL);
  }

  move(10, 0);
  for (; self != NULL; self = self->next) {
    char *dot = strrchr(self->name, '.');
    if (dot == NULL)
      continue;
    for (int i = 0; i < ext_s; i++) {
      if (!strcmp(ext[i].ext, dot)) {
        tmp[i] = tmp[i]->next = file_new(self->name);
        break;
      }
    }
  }

  for (int i = 0; i < ext_s; i++) {
    printw("\nExtension %s: ", ext[i].ext);
    file_T n = ext[i].list->next;
    free(ext[i].list);
    file_list(ext[i].list = n);
  }
  free(tmp);
}

#ifdef _WIN32
#define WINDOWS       1
#define UNIX          0
#define SETTINGS_DIR  "C:\\GoL" // without trailing '\'
#define MAKE_DIR(dir) (mkdir(dir))
#else
#define WINDOWS       0
#define UNIX          1
#define SETTINGS_DIR  "/home/magaknuto/GoL"
#define MAKE_DIR(dir) (mkdir(dir, 0777))
#endif // _WIN32

void setup(char *pass) {
  if (!DirectoryExists(SETTINGS_DIR)) {
    printw("%s does not exist, trying to create it....\n", SETTINGS_DIR);
    if (MAKE_DIR(SETTINGS_DIR) != 0) {
      printw("Unfortunately, can't create a directory: %s", SETTINGS_DIR);
      getch();
      exit(1);
    }
    printw("Success!\n");
  } else
    printw("%s already exists\n", SETTINGS_DIR);

  if (chdir(SETTINGS_DIR) != 0) {
    printw("chdir(%s): not successfull!\n", SETTINGS_DIR);
    getch();
    exit(1);
  } else
    printw("Directory has successfully been changed to %s!\n", SETTINGS_DIR);
}

#define MAX(a, b)      ((a > b) ? a : b)
#define MIN(a, b)      ((a < b) ? a : b)
#define CLAMP(a, x, y) (a = MAX(x, MIN(a, y)))

void cursor_offset(int oy, int ox) {
  int y, x;
  getyx(stdscr, y, x);
  move(y + oy, x + ox);
}

typedef int (*input_f)(int);
void input(char *buffer, int size, input_f crit) {
  int ch;
  int read = 0;
  while ((ch = getch()) != '\n') {
    switch (ch) {
    case KEY_BACKSPACE:
      if (read > 0) {
        cursor_offset(0, -1);
        delch();
        read--;
      }
    default:
      if (read < size && crit && crit(ch)) {
        printw("%c", ch);
        buffer[read++] = ch;
      }
    }
  }
  printw("\n");
  buffer[read] = '\0';
}

void wcenter_horizontal(WINDOW *win, int y, int n) {
  int mx, my;
  getmaxyx(win, my, mx);
  wmove(win, y, (mx - n) / 2);
}

int wcenter_vertical(WINDOW *win, int n) {
  int mx, my;
  getmaxyx(win, my, mx);
  return (my - n) / 2;
}
#define center_vertical(n)      wcenter_vertical(stdscr, n);
#define center_horizontal(y, n) wcenter_horizontal(stdscr, y, n);

void show(WINDOW *win, unsigned **univ, int w, int h, int y, int x) {
  attrset(COLOR_PAIR(0));
  int ph = h, pw = w, mh, mw;
  getmaxyx(win, mh, mw);
  CLAMP(ph, 0, mh);
  CLAMP(pw, 0, mw);
  int top = wcenter_vertical(win, ph);

  for (int i = 0; i < ph; i++) {
    wcenter_horizontal(win, top + i, pw);
    for (int j = 0; j < pw; j++) {
#ifdef _WIN32
      wattrset(win,
               COLOR_PAIR((univ[(i + y + h) % h][(j + x + w) % w]) ? 2 : 3));
      wprintw(win, "  ");
#else
      /* if (univ[(i + y + h) % h][(j + x + w) % w]) */
      /*   wprintw(win, "%lc", 0x2B1B); */
      /* else */
      /*   wprintw(win, "  ", 0x2B1C); */
      if (univ[(i + y + h) % h][(j + x + w) % w])
        wprintw(win, "%lc", (wchar_t)L'\u2B1B');
      else
        wprintw(win, "  ", (wchar_t)L'\u2B1C');
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

void game(int w, int h) {
  clear();
  box(stdscr, ACS_VLINE, ACS_HLINE);
  refresh();

  box(game_w, ACS_VLINE, ACS_HLINE);
  wrefresh(game_w);

  box(status_w, ACS_VLINE, ACS_HLINE);
  wbkgd(status_w, '.');
  wrefresh(status_w);

  if (!w || !h) {
    getmaxyx(board_w, h, w);
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

  nodelay(status_w, 1);
  keypad(status_w, TRUE);
  int oy = 0, ox = 0;
  int in = 3;
  int gen_step = 1;
  while (1) {
    show(board_w, univ, w, h, oy, ox);
    evolve(univ, new, w, h, gen_step);
    for (int i = 0; i < in; i++) {
      int c = wgetch(status_w);
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
        break;
      }
    }
    ox = (ox + w) % w;
    oy = (oy + h) % h;
    flushinp();
    usleep(150000);
  }
}

void start(char *pass) {
  char buffer[100];
  int  row, column;
  printw("Unesi broj redova: ");
  input(buffer, 4, isdigit);
  row = atoi(buffer);
  printw("Unesi broj kolona: ");
  input(buffer, 4, isdigit);
  column = atoi(buffer);
  game(row, column);
  refresh();
}

void load(char *pass) {
  printw("Load\n");
  getch();
}

void exitp(char *pass) {
  endwin();
  exit(0);
}

struct extension_T ext[] = {
    {   ".c", NULL},
    {   ".h", NULL},
    {".part", NULL},
    { ".all", NULL}
};
int ext_s = sizeof(ext) / sizeof(struct extension_T);

void list(char *pass) {
  file_T files = file_fromDirectory();
  file_filter(files, ext, ext_s);
  for (int i = 0; i < ext_s; i++)
    file_free(ext[i].list);
  file_free(files);
}

struct menu_T {
  void (*callback)(char *);
  char *name;
};

struct menu_T menu_items[] = {
    {start, "Start"},
    {setup, "Setup"},
    { list,  "List"},
    { load,  "Load"},
    {exitp,  "Exit"}
};
int menu_items_s = sizeof(menu_items) / sizeof(struct menu_T);

void display_menu(struct menu_T *items, int size) {
  int current = 0;

  int *lens = malloc(size * sizeof(int));
  for (int i = 0; i < size; i++)
    lens[i] = strlen(items[i].name);

  int y_offset = center_vertical(size * 2);
  while (TRUE) {
    CLAMP(current, 0, size - 1);
    for (int i = 0; i < size; i++) {
      attrset(COLOR_PAIR(i == current ? 1 : 0));
      center_horizontal(y_offset + i * 2, lens[i]);
      printw("%s", items[i].name);
    }
    refresh();
    switch (getch()) {
    case 'k':
    case KEY_UP:
      current--;
      break;
    case KEY_DOWN:
    case 'j':
      current++;
      break;
    case '\n':
      move(16, 0);
      attrset(COLOR_PAIR(0));
      items[current].callback(NULL);
    }
  }
  free(lens);
}

void ncurses_window_setup(void) {
  game_w = newwin(LINES - 2 - top_space, COLS - 4, 1 + top_space, 2);
  board_w = newwin(LINES - 4 - top_space, COLS - 6, 2 + top_space, 3);
  status_w = newwin(top_space, COLS - 4, 1, 2);
}

void ncurses_window_end(void) {
  delwin(game_w);
  delwin(board_w);
  delwin(status_w);
}

void ncurses_start(void) {
  initscr();
  start_color();
  use_default_colors();
  curs_set(0);
  noecho();
  keypad(stdscr, TRUE);
  init_pair(0, COLOR_WHITE, -1);
  init_pair(1, COLOR_RED, -1);

  init_pair(2, COLOR_WHITE, COLOR_WHITE);
  init_pair(3, COLOR_BLACK, -1);
}

int main(void) {
  setlocale(LC_ALL, "");
  ncurses_start();
  ncurses_window_setup();

  display_menu(menu_items, menu_items_s);
  getch();

  endwin();

  return 0;
}
