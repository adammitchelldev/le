#ifndef __LE_TERMINAL_H__
#define __LE_TERMINAL_H__

void die(const char *s);

void disableRawMode();

void enableRawMode();

int editorReadKey();

int getCursorPosition(int *rows, int *cols);

int getWindowSize(int *rows, int *cols);

#endif
