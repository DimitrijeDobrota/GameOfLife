#ifndef UTILS_H
#define UTILS_H

#include <curses.h>

#include "display.h"

#define MAX(a, b)       ((a > b) ? a : b)
#define MIN(a, b)       ((a < b) ? a : b)
#define CLAMP(a, x, y)  ((a) = (MAX(x, MIN(a, y))))
#define ACLAMP(a, x, y) (MAX(x, MIN(a, y)))
#define WCLAMP(a, x)    ((a + x) % x)

#ifdef _WIN32
#define is_term_resized(a, b) is_termresized()
#define usleep(a)             _sleep(0.5)
#define HANDLE_RESIZE                                                          \
  {                                                                            \
    resize_term(0, 0);                                                         \
    handle_winch(10);                                                          \
  }
#else
#define HANDLE_RESIZE                                                          \
  { handle_winch(10); }
#endif

#ifdef NO_UNICODE
#define UNICODE 0
#else
#define UNICODE 1
#endif

#define MAX_SCREEN_H 512
#define MAX_SCREEN_W 1888

#define MEM_CHECK(x)                                                           \
  if ((x) == NULL) {                                                           \
    display_stop();                                                            \
    printf("MEM ERROR");                                                       \
    abort();                                                                   \
  }

#define FILE_CHECK(x)                                                          \
  if ((x) == NULL) {                                                           \
    display_stop();                                                            \
    printf("FILE ERROR");                                                      \
    abort();                                                                   \
  }

#endif
