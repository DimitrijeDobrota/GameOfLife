#ifndef UTILS_H
#define UTILS_H

#include <curses.h>

#define MAX(a, b)      ((a > b) ? a : b)
#define MIN(a, b)      ((a < b) ? a : b)
#define CLAMP(a, x, y) ((a) = (MAX(x, MIN(a, y))))
#define ACLAMP(a, x, y) (MAX(x, MIN(a, y)))

#ifdef _WIN32
#define is_term_resized(a, b) is_termresized()
#define usleep(a)             _sleep(0.5)
#define HANDLE_RESIZE                                                          \
  {                                                                            \
    resize_term(0, 0);                                                         \
    handle_winch(10);                                                          \
    goto redraw;                                                               \
  }
#else
#define HANDLE_RESIZE                                                          \
  {                                                                            \
    handle_winch(10);                                                          \
    goto redraw;                                                               \
  }
#endif
#endif
