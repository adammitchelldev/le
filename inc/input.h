#ifndef __LE_INPUT_H__
#define __LE_INPUT_H__

char *editorPrompt(const char *prompt, void (*callback)(char *, int, int, void *), void *state);

void editorMoveCursor(int key);

void editorProcessKeypress();

#endif
