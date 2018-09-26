/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <terminal.h>
#include <file.h>
#include <output.h>
#include <input.h>
#include <events.h>
#include <editor.h>

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

  luaInit();
  luaExecuteScript("init.lua");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
