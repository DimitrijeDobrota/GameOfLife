/**
 * @file file.h
 * @author Dimitrije Dobrota
 * @date 19 June 2022
 * @brief File interface
 */

#ifndef FILE_H
#define FILE_H

#include "display.h"

int  file_setup(void);
void load_files(void);
void free_files(void);
int  file_select_extension(char *ext, char ***buffer);

void file_load_pattern(char *name, int index);
void file_save_pattern(char *name, int index);
void file_load(char *name, int index);
void file_save(char *name, int index);

#endif
