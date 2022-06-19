/**
 * @file utils.h
 * @author Dimitrije Dobrota
 * @date 19 June 2022
 * @brief Common macros for use in other modules
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

/// maximum of the two numbers
#define MAX(a, b) ((a > b) ? a : b)

/// minimum of the two numbers
#define MIN(a, b) ((a < b) ? a : b)

/// clamp the number a between x and y
#define CLAMP(a, x, y) ((a) = (MAX(x, MIN(a, y))))

/// clamp the value of a between x and y without assignment
#define ACLAMP(a, x, y) (MAX(x, MIN(a, y)))

/// clamp with wrapping
#define WCLAMP(a, x) ((a + x) % x)

/// Check if compiled with Unicode support
#ifdef NO_UNICODE
#define UNICODE 0
#else
#define UNICODE 1
#endif

static void err(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(1);
}

/// Check for memory error, and abort
#define MEM_CHECK(x)                                                           \
  if ((x) == NULL)                                                             \
    err("MEMORY ERROR");

/// Check for file error, and abort
#define FILE_CHECK(x)                                                          \
  if ((x) == NULL)                                                             \
    err("MEMORY ERROR");

#endif
