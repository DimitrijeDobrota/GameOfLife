#include <ctype.h>
#include <curses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "file.h"
#include "game.h"
#include "logic.h"
#include "utils.h"
#include "window.h"

extern window_T MAIN_w;
window_T        menu_w;

void settings(char *pass, int index) {
  struct imenu_T imenu_items[] = {
      {   "Number of rows", 9, isdigit, NULL},
      {"Number of columns", 9, isdigit, NULL},
  };
  int imenu_items_s = sizeof(imenu_items) / sizeof(struct imenu_T);

  window_set_title(menu_w, "Game settings");
  while (display_imenu(menu_w, imenu_items, imenu_items_s)) {
    int row = atoi(imenu_items[0].buffer);
    int column = atoi(imenu_items[1].buffer);

    game(row, column, index);
    break;
  }

  for (int i = 0; i < imenu_items_s; i++) {
    free(imenu_items[i].buffer);
    imenu_items[i].buffer = NULL;
  }
}

void mode_select(char *pass, int index) {
  struct menu_T *mode_items;

  MEM_CHECK(mode_items = malloc(evolution_size * sizeof(struct menu_T)));
  for (int i = 0; i < evolution_size; i++) {
    mode_items[i].name = evolution_names[i];
    mode_items[i].callback = settings;
  }
  display_menu(menu_w, "Game Mode", mode_items, evolution_size, 0);

  free(mode_items);
}

void (*file_save_method)(char *, int);

void new_file(char *pass, int index) {
  struct imenu_T new_file_items[] = {
      {"Pick a name", 10, isalnum, NULL},
  };
  int new_file_items_s = sizeof(new_file_items) / sizeof(struct imenu_T);

  window_set_title(menu_w, "New File");
  while (display_imenu(menu_w, new_file_items, new_file_items_s)) {
    file_save_method(new_file_items[0].buffer, index);
    break;
  }

  for (int i = 0; i < new_file_items_s; i++) {
    free(new_file_items[i].buffer);
    new_file_items[i].buffer = NULL;
  }
}

struct menu_T *file_menu_list(char *ext, void (*callback)(char *, int),
                              int offset, int *size) {
  struct menu_T *file_items;
  char         **buffer;
  int            n;

  load_files();
  n = file_select_extension(ext, &buffer);

  MEM_CHECK(file_items = malloc((n + offset) * sizeof(struct menu_T)));
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

  file_save_method = file_save_pattern;
  display_menu(menu_w, "Save Pattern", file_items, n, 0);
  free(file_items);
}

void load_pattern(void) {
  int            n;
  struct menu_T *file_items = file_menu_list("part", file_load_pattern, 0, &n);

  display_menu(menu_w, "Load Pattern", file_items, n, 0);
  free(file_items);
}

void load(char *pass, int index) {
  int            n;
  struct menu_T *file_items = file_menu_list("all", file_load, 0, &n);

  display_menu(menu_w, "Load Game", file_items, n, 0);
  free(file_items);
}

void save(void) {
  int            n;
  struct menu_T *file_items = file_menu_list("all", file_save, 1, &n);
  file_items[0].name = "NEW";
  file_items[0].callback = new_file;

  file_save_method = file_save;
  display_menu(menu_w, "Save Pattern", file_items, n, 0);
  free(file_items);
}

void help(char *pass, int index) {
  window_set_title(menu_w, "Help");
  display_patterns(menu_w);
}

void exitp(char *pass, int index) { exit(0); }

struct menu_T menu_items[] = {
    {mode_select, "Start"},
    {       load,  "Load"},
    {       help,  "Help"},
    {      exitp,  "Exit"}
};

int menu_items_s = sizeof(menu_items) / sizeof(struct menu_T);

int main(void) {
  setlocale(LC_ALL, "");
  atexit(display_stop);

  file_setup();
  display_start();

  menu_w = MAIN_w;
  while (TRUE) {
    display_menu(menu_w, "Main menu", menu_items, menu_items_s, 1);
  }

  return 0;
}
