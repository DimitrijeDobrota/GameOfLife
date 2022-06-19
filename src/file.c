/**
 * @file file.c
 * @author Dimitrije Dobrota
 * @date 16 June 2022
 * @brief This file contains functions for handling save files
 *
 * This file aims to provide a simple interface for interacting with the
 * file system on Linux and Windows systems. After proper game directory has
 * been selected, functions implemented here will read the list of files, filter
 * them based on the extension, create new files for storing a whole game or
 * just a pattern.
 */

#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
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

/**
 * @brief Check if a directory at specific path exists
 */
int DirectoryExists(const char *path) {
  struct stat stats;
  stat(path, &stats);

  return S_ISDIR(stats.st_mode);
}

/**
 * @brief Try to change the directory to SETTINGS_DIR, if it doesn't exist
 * create it. Return 0 on failure.
 */
void file_setup(void) {
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
    if (MAKE_DIR(dir) != 0)
      err("Cannot create the directory %s", dir);
  }

  if (chdir(dir) != 0) {
    printf("Cannot change the directory\n");
    if (MAKE_DIR(dir) != 0)
      err("Cannot change directory to %s", dir);
  }

  free(dir);
}

typedef struct file_T *file_T;

/**
 * @brief A node in a linked list of file names
 */
struct file_T {
  file_T next; ///< pointer to the next node
  char  *name; ///< name of a file
};

static file_T loaded_files;

/**
 * @brief create new file_T
 *
 * Allocate the memory for a new file_T. The name field will be set to NULL if
 * NULL is provided as name, otherwise it will be set to a DUPLICATE of a
 * string provided.
 */
file_T file_new(char *name) {
  file_T f;
  MEM_CHECK(f = calloc(1, sizeof(*f)));

  if (name != NULL) {
    MEM_CHECK(f->name = malloc((strlen(name) + 1) * sizeof(char)));
    strcpy(f->name, name);
  }

  return f;
}

/**
 * @brief Free the linked list of file_T
 */
void file_free(file_T self) {
  if (self == NULL)
    return;

  file_free(self->next);
  free(self->name);
  free(self);
}

/**
 * @brief Add file of a specific name to the begin of a linked list
 *
 * First item of the list will be skipped and unchanged.
 */
file_T file_add(file_T self, char *name) {
  file_T new = file_new(name);
  new->next = self->next;
  self->next = new;
  return new;
}

/**
 * @brief Sort linked list in a lexicographical order
 */
void file_sort(file_T self) {
  for (file_T p = self->next; p; p = p->next)
    for (file_T t = p->next; t; t = t->next)
      if (strcmp(p->name, t->name) > 0) {
        char *tmp = p->name;
        p->name = t->name;
        t->name = tmp;
      }
}

/**
 * @brief Return a new linked list of files contained in the current directory
 */
file_T file_fromDirectory(void) {
  file_T base = file_new(NULL);

  struct dirent *de;
  DIR           *dr = opendir(".");
  if (dr == NULL)
    return NULL;

  while ((de = readdir(dr)) != NULL)
    file_add(base, de->d_name);

  file_sort(base);
  file_T n = base->next;

  free(base);
  free(dr);

  return n;
}

/**
 * @brief Check if file is in the linked list, return NULL if it's not
 */
file_T file_find(file_T self, char *name) {
  for (; self != NULL; self = self->next) {
    if (!strcmp(self->name, name))
      return self;
  }
  return NULL;
}

/**
 * @brief Return array of the filenames that have the specific extension
 *
 * Memory for the buffer is allocated automatically and should be freed with
 * free() after it's no longer needed.
 */
int file_select_extension(char *ext, char ***buffer) {
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

/**
 * @brief Fill loaded_files linked list with files in the current directory,
 * freeing any previous lists
 */
void load_files(void) {
  if (loaded_files)
    file_free(loaded_files);
  loaded_files = file_fromDirectory();
}

/**
 * @brief Free loaded_files linked list
 */
void free_files(void) { file_free(loaded_files); }

// from logic.c
extern Cell **save_cells;   ///< List of Cells to be saved in a pattern
extern int    save_cells_s; ///< Size of save_cells
extern int    pos_y;        ///< Real cursor y coordinate
extern int    pos_x;        ///< Real cursor x coordinate
extern int    evolve_index; ///< index of the current game mode

// form game.c
extern int height; ///< height of the current game
extern int width;  ///< width of the current game

/**
 * @brief Load a pattern to the current cursor position from a file with name
 * and extension .part
 */
void file_load_pattern(char *name, int index) {
  FILE *f;
  char *fname;
  int   min_y = INT_MAX, min_x = INT_MAX, max_y = -1, max_x = -1;
  int   row, col, val;

  MEM_CHECK(fname = malloc((strlen(name) + 5) * sizeof(char)));
  sprintf(fname, "%s.part", name);

  FILE_CHECK(f = fopen(fname, "r"));

  while (fscanf(f, "%d %d %d", &row, &col, &val) != EOF) {
    min_y = MIN(min_y, row);
    min_x = MIN(min_x, col);
    max_y = MAX(max_y, row);
    max_x = MAX(max_x, col);
  }

  for (int i = min_y; i <= max_y; i++)
    for (int j = min_x; j <= max_x; j++)
      setAt(i + pos_y, j + pos_x, 0);

  rewind(f);
  while (fscanf(f, "%d %d %d", &row, &col, &val) != EOF)
    if (height != 0 && width != 0)
      setAt(WCLAMP(pos_y + row, height), WCLAMP(pos_x + col, width), val);
    else
      setAt(pos_y + row, pos_x + col, val);
}

/**
 * @brief Save a pattern of cells referenced in save_cells array to a file with
 * name and extension .part
 */
void file_save_pattern(char *name, int index) {
  FILE *f;
  char *fname;

  MEM_CHECK(fname = malloc((strlen(name) + 6) * sizeof(char)));
  sprintf(fname, "%s.part", name);

  FILE_CHECK(f = fopen(fname, "w"));

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

/**
 * @brief Load the game from the file with name and extension .all
 */
void file_load(char *name, int index) {
  FILE *f;
  char *fname;
  int   w, h;

  MEM_CHECK(fname = malloc((strlen(name) + 5) * sizeof(char)));
  sprintf(fname, "%s.all", name);

  FILE_CHECK(f = fopen(fname, "r"));

  fscanf(f, "%d %d %d", &h, &w, &evolve_index);
  int row, col, val;
  while (fscanf(f, "%d %d %d", &row, &col, &val) != EOF) {
    setAt(row, col, val);
  }

  game(h, w, evolve_index);
}

/**
 * @brief Save the current game to the file with name and extension .all
 */
void file_save(char *name, int index) {
  FILE *f;
  char *fname;

  MEM_CHECK(fname = malloc((strlen(name) + 5) * sizeof(char)));
  sprintf(fname, "%s.all", name);

  FILE_CHECK(f = fopen(fname, "w"));

  fprintf(f, "%d %d %d\n", height, width, evolve_index);
  for (Cell *c = hash; c != NULL; c = c->hh.next) {
    fprintf(f, "%d %d %d\n", c->cord.row, c->cord.col, c->val);
  }

  fclose(f);
}
