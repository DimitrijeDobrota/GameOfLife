#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "display.h"
#include "game.h"
#include "logic.h"
#include "utils.h"

#ifdef _WIN32
#define SETTINGS_DIR  "C:\\GoL" // without trailing '\'
#define MAKE_DIR(dir) (mkdir(dir))
#else
#define SETTINGS_DIR  "GoL"
#define MAKE_DIR(dir) (mkdir(dir, 0777))
#endif // _WIN32

typedef struct file_T *file_T;
struct file_T {
  file_T next;
  char  *name;
};

file_T loaded_files;

file_T file_new(char *name) {
  file_T f;
  MEM_CHECK(f = malloc(sizeof(*f)));
  f->next = NULL;
  if (name == NULL) {
    f->name = NULL;
    return f;
  }

  MEM_CHECK(f->name = malloc((strlen(name) + 1) * sizeof(char)));
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

int file_setup(void) {
  char *dir;
  MEM_CHECK(dir = malloc(PATH_MAX * sizeof(char)));

#ifdef _WIN32
  strcpy(dir, SETTINGS_DIR);
#else
  const char *homedir = getenv("HOME");
  sprintf(dir, "%s/%s", homedir, SETTINGS_DIR);
#endif

  if (!DirectoryExists(dir)) {
    printf("Directory %s does not exists; Trying to create it...\n", dir);
    if (MAKE_DIR(dir) != 0) {
      printf("Cannot create the directory\n");
      return 0;
    }
  }

  if (chdir(dir) != 0) {
    printf("Cannot change the directory\n");
    return 0;
  }
  return 1;
}

int file_select(char *ext, char ***buffer) {
  int    maxsize = 4;
  int    size = 0;
  char **tmp = NULL;
  file_T current = loaded_files;

  MEM_CHECK(*buffer = malloc(maxsize * sizeof(char *)));
  for (; current != NULL; current = current->next) {
    char *dot = strrchr(current->name, '.');
    if (dot == NULL)
      continue;
    if (!strcmp(dot + 1, ext)) {
      *dot = '\0';
      (*buffer)[size++] = current->name;
      if (size == maxsize) {
        maxsize *= 2;
        MEM_CHECK(tmp = realloc(*buffer, maxsize * sizeof(char *)));
        *buffer = tmp;
      }
    }
  }
  if (size) {
    MEM_CHECK(tmp = realloc(*buffer, size * sizeof(char *)));
    *buffer = tmp;
  }

  return size;
}

void load_files(void) {
  if (loaded_files)
    file_free(loaded_files);
  loaded_files = file_fromDirectory();
}

void free_files(void) { file_free(loaded_files); }

// from logic.c
extern Cell **save_cells;
extern int    save_cells_s, pos_y, pos_x, evolve_index;

// form game.c
extern int width, height;

void file_load_pattern(char *name, int index) {
  char *fname;

  MEM_CHECK(fname = malloc((strlen(name) + 5) * sizeof(char)));
  sprintf(fname, "%s.part", name);

  FILE *f = fopen(fname, "r");
  if (!f)
    exit(1);

  int row, col, val;
  while (fscanf(f, "%d %d %d", &row, &col, &val) != EOF) {
    setAt(pos_y + row, pos_x + col, val);
  }
}

void file_save_pattern(char *name, int index) {
  char *fname;

  MEM_CHECK(fname = malloc((strlen(name) + 5) * sizeof(char)));
  sprintf(fname, "%s.part", name);

  FILE *f = fopen(fname, "w");
  if (!f)
    exit(1);

  int min_y = save_cells[0]->cord.row;
  int min_x = save_cells[0]->cord.col;
  for (int i = 0; i < save_cells_s; i++) {
    Cell *c = save_cells[i];
    min_y = MIN(min_y, c->cord.row);
    min_x = MIN(min_x, c->cord.col);
  }

  for (int i = 0; i < save_cells_s; i++) {
    Cell *c = save_cells[i];
    fprintf(f, "%d %d %d\n", c->cord.row - min_y, c->cord.col - min_x, c->val);
  }

  fclose(f);
}

void file_load(char *name, int index) {
  char *fname;
  int   w, h;

  MEM_CHECK(fname = malloc((strlen(name) + 5) * sizeof(char)));
  sprintf(fname, "%s.all", name);

  FILE *f = fopen(fname, "r");
  if (!f)
    exit(1);

  fscanf(f, "%d %d %d", &h, &w, &evolve_index);
  int row, col, val;
  while (fscanf(f, "%d %d %d", &row, &col, &val) != EOF) {
    setAt(pos_y + row, pos_x + col, val);
  }

  game(h, w, evolve_index);
}

void file_save(char *name, int index) {
  char *fname;

  MEM_CHECK(fname = malloc((strlen(name) + 5) * sizeof(char)));
  sprintf(fname, "%s.all", name);

  FILE *f = fopen(fname, "w");
  if (!f)
    exit(1);

  fprintf(f, "%d %d %d\n", height, width, evolve_index);
  for (Cell *c = hash; c != NULL; c = c->hh.next) {
    fprintf(f, "%d %d %d\n", c->cord.row, c->cord.col, c->val);
  }

  fclose(f);
}
