#ifndef __INPUT__

char *editorPrompt(char *prompt, void (*callback)(char *, int, int, void *), void *state);

void editorMoveCursor(int key);

void editorProcessKeypress();

#define __INPUT__
#endif
