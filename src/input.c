/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <global.h>

#include <terminal.h>
#include <row.h>
#include <operations.h>
#include <file.h>
#include <buffer.h>
#include <output.h>
#include <input.h>
#include <events.h>
#include <editor.h>
#include <find.h>

char *editorPrompt(const char *prompt, void (*callback)(char *, int, int, void *), void *state) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';
  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();

    int c = editorReadKey();
    switch(c) {

      case '\x1b':
      case CTRL_KEY('q'):
        editorSetStatusMessage("");
        if (callback) callback(buf, buflen, c, state);
        free(buf);
        return NULL;
        break;

      case '\r':
        if (buflen != 0) {
          editorSetStatusMessage("");
          if (callback) callback(buf, buflen, c, state);
          return buf;
        }
        break;

      case BACKSPACE:
        if (buflen != 0) {
          buf[--buflen] = '\0';
        }
        break;

      default:
        if (!iscntrl(c) && c < 128) {
          if (buflen == bufsize - 1) {
            bufsize *= 2;
            buf = realloc(buf, bufsize);
          }
          buf[buflen++] = c;
          buf[buflen] = '\0';
        }
        break;
    }

    if (callback) callback(buf, buflen, c, state);
  }
}

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch (key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        E.cx--;
      } else if (E.cy > 0) {
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case ARROW_RIGHT:
      if (row && E.cx < row->size) {
        E.cx++;
      } else if (E.cy < E.numrows) {
        E.cy++;
        E.rx = 0;
      }
      break;
    case ARROW_UP:
      if (E.cy != 0) {
        E.cy--;
        if(E.cy < E.numrows) {
          row = &E.row[E.cy];
          E.cx = editorRowRxToCx(row, E.rx);
        }
      }
      break;
    case ARROW_DOWN:
      if (E.cy < E.numrows) {
        E.cy++;
        if(E.cy < E.numrows) {
          row = &E.row[E.cy];
          E.cx = editorRowRxToCx(row, E.rx);
        }
      }
      break;
  }

  // TODO don't repeat this in the case that vertical is jumped to
  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

void editorProcessKeypress() {
  static int quit_times = LE_QUIT_TIMES;

  int c = editorReadKey();

  if(luaEventKeypress(c)) {
    editorScroll();
    quit_times = LE_QUIT_TIMES;
    return;
  }

  switch (c) {
    case '\r':
      editorInsertNewline();
      editorScroll();
      break;

    case CTRL_KEY('q'):
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("WARNING!!! File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case CTRL_KEY('s'):
      editorSave();
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      editorDelChar();
      editorScroll();
      break;

    case HOME_KEY:
      E.cx = 0;
      editorScroll();
      break;

    case END_KEY:
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      editorScroll();
      break;

    case CTRL_KEY('f'):
      editorFind();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP) {
          E.cy = E.rowoff;
        } else if (c == PAGE_DOWN) {
          E.cy = E.rowoff + E.screenrows - 1;
          if (E.cy > E.numrows) E.cy = E.numrows;
        }
        int times = E.screenrows;
        while (times--)
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        editorScroll();
      }
      break;

    case ARROW_LEFT:
    case ARROW_RIGHT:
    case ARROW_UP:
    case ARROW_DOWN:
      editorMoveCursor(c);
      editorScroll();
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;

    case '\t':
      goto insert;

    default:
      if (!iscntrl(c) && c < 128) {
      insert:
        editorInsertChar(c);
        editorScroll();
      }
      break;
  }

  quit_times = LE_QUIT_TIMES;
}
