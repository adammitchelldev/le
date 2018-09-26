#ifndef __LE_FILE_H__
#define __LE_FILE_H__

#include "global.h"

#include "row.h"

char *editorRowsToString(int *buflen);

void editorOpen(char *filename);

void editorSave();

#endif
