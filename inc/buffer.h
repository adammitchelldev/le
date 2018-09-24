#ifndef __BUFFER__

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len);

void abFree(struct abuf *ab);

#define __BUFFER__
#endif