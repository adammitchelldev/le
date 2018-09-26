#ifndef __LE_BUFFER_H__
#define __LE_BUFFER_H__

#include <stdlib.h>

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len);

void abFree(struct abuf *ab);

#endif
