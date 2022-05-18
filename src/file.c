#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "display.h"

#ifdef _WIN32
#define SETTINGS_DIR  "C:\\GoL" // without trailing '\'
#define MAKE_DIR(dir) (mkdir(dir))
#else
#define SETTINGS_DIR  "/home/magaknuto/GoL"
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
  if (!DirectoryExists(SETTINGS_DIR)) {
    if (MAKE_DIR(SETTINGS_DIR) != 0) {
      return 0;
    }
  }

  if (chdir(SETTINGS_DIR) != 0) {
    return 0;
  }
  return 1;
}

int file_select(char *ext, char ***buffer) {
  int    maxsize = 4;
  int    size = 0;
  char **tmp;
  file_T current = loaded_files;

  *buffer = malloc(maxsize * sizeof(char *));
  for (; current != NULL; current = current->next) {
    char *dot = strrchr(current->name, '.');
    if (dot == NULL)
      continue;
    if (!strcmp(dot + 1, ext)) {
      *dot = '\0';
      (*buffer)[size++] = current->name;
      if (size == maxsize) {
        maxsize *= 2;
        tmp = realloc(*buffer, maxsize * sizeof(char *));
        if (!tmp)
          exit(1);
        *buffer = tmp;
      }
    }
  }
  tmp = realloc(*buffer, size * sizeof(char *));
  if (!tmp)
    exit(1);
  *buffer = tmp;
  return size;
}

void load_files(void) { loaded_files = file_fromDirectory(); }
void free_files(void) { file_free(loaded_files); }
void load_file(window_T win, char *pass) { (void)3; }
