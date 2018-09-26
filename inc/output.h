#ifndef __LE_OUTPUT_H__
#define __LE_OUTPUT_H__

#include "buffer.h"

void editorScroll();

void editorDrawRows(struct abuf *ab);

void editorDrawStatusBar(struct abuf *ab);

void editorDrawMessageBar(struct abuf *ab);

void editorRefreshScreen();

void editorSetStatusMessage(const char *fmt, ...);

#endif
