#ifndef __LE_FILE__

#include <global.h>

#include <row.h>

char *editorRowsToString(int *buflen);

void editorOpen(char *filename);

void editorSave();

#define __LE_FILE__
#endif
