#ifndef FILE_H
#define FILE_H

#include "display.h"

int  file_setup(void);
void load_files(void);
void free_files(void);
int  file_select(char *ext, char ***buffer);

void load_file(window_T win, char *pass, int index);

#endif
