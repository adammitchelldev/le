/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <global.h>

#include <terminal.h>

/***
########  ########   #######  ########  #######  ######## ##    ## ########  ########  ######
##     ## ##     ## ##     ##    ##    ##     ##    ##     ##  ##  ##     ## ##       ##    ##
##     ## ##     ## ##     ##    ##    ##     ##    ##      ####   ##     ## ##       ##
########  ########  ##     ##    ##    ##     ##    ##       ##    ########  ######    ######
##        ##   ##   ##     ##    ##    ##     ##    ##       ##    ##        ##             ##
##        ##    ##  ##     ##    ##    ##     ##    ##       ##    ##        ##       ##    ##
##        ##     ##  #######     ##     #######     ##       ##    ##        ########  ######
 ***/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
void editorScroll();
char *editorPrompt(char *prompt, void (*callback)(char *, int, int, void *), void *state);

int luaEventKeypress(int c);

/***
######## ######## ########  ##     ## #### ##    ##    ###    ##
   ##    ##       ##     ## ###   ###  ##  ###   ##   ## ##   ##
   ##    ##       ##     ## #### ####  ##  ####  ##  ##   ##  ##
   ##    ######   ########  ## ### ##  ##  ## ## ## ##     ## ##
   ##    ##       ##   ##   ##     ##  ##  ##  #### ######### ##
   ##    ##       ##    ##  ##     ##  ##  ##   ### ##     ## ##
   ##    ######## ##     ## ##     ## #### ##    ## ##     ## ########
 ***/



/***
########   #######  ##      ##     #######  ########   ######
##     ## ##     ## ##  ##  ##    ##     ## ##     ## ##    ##
##     ## ##     ## ##  ##  ##    ##     ## ##     ## ##
########  ##     ## ##  ##  ##    ##     ## ########   ######
##   ##   ##     ## ##  ##  ##    ##     ## ##              ##
##    ##  ##     ## ##  ##  ##    ##     ## ##        ##    ##
##     ##  #######   ###  ###      #######  ##         ######
 ***/

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (LE_TAB_STOP - 1) - (rx % LE_TAB_STOP);
    rx++;
  }
  return rx;
}

int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (LE_TAB_STOP - 1) - (cur_rx % LE_TAB_STOP);
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}

void editorUpdateRow(erow *row) {
  int tabs = 0;

  char *chars = row->chars;
  char *end = chars + row->size;
  for (; chars != end; chars++)
    if (*chars == '\t') tabs++;

  free(row->render);
  char *render = malloc(row->size + tabs*(LE_TAB_STOP - 1) + 1);
  row->render = render;

  int idx = 0;
  chars = row->chars;
  for (; chars != end; chars++) {
    char c = *chars;
    switch(c) {
      case '\t':
        *render = ' ';
        render++;
        idx++;
        while (idx % LE_TAB_STOP != 0) {
          *render = ' ';
          render++;
          idx++;
        }
        break;
      default:
        *render = c;
        render++;
        idx++;
        break;
    }
  }
  *render = '\0';
  row->rsize = idx;
}

// TODO array indexing vs pointers
void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows) return;
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  editorUpdateRow(E.row + at); // Pointer arithmetic

  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
}

// TODO yuck, linkedlists maybe?
void editorDelRow(int at) {
  if (at < 0 || at >= E.numrows) return;
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}



/***
######## ########  #### ########     #######  ########   ######
##       ##     ##  ##     ##       ##     ## ##     ## ##    ##
##       ##     ##  ##     ##       ##     ## ##     ## ##
######   ##     ##  ##     ##       ##     ## ########   ######
##       ##     ##  ##     ##       ##     ## ##              ##
##       ##     ##  ##     ##       ##     ## ##        ##    ##
######## ########  ####    ##        #######  ##         ######
 ***/

void editorInsertChar(int c) {
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(E.row + E.cy, E.cx, c);
  E.cx++;
}

void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorDelChar() {
  if (E.cy == E.numrows) return;
  if (E.cx == 0 && E.cy == 0) return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDelChar(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;
  }
}

/***
######## #### ##       ########    ####       ##  #######
##        ##  ##       ##           ##       ##  ##     ##
##        ##  ##       ##           ##      ##   ##     ##
######    ##  ##       ######       ##     ##    ##     ##
##        ##  ##       ##           ##    ##     ##     ##
##        ##  ##       ##           ##   ##      ##     ##
##       #### ######## ########    #### ##        #######
 ***/

char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < E.numrows; j++)
    totlen += E.row[j].size + 1;
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < E.numrows; j++) {
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename);

  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    if (linelen != -1) {
      while (linelen > 0 && (line[linelen - 1] == '\n' ||
                             line[linelen - 1] == '\r'))
        linelen--;
      editorInsertRow(E.numrows, line, linelen);
    }
  }
  free(line);
  fclose(fp);
  E.dirty = 0;
}

// TODO save with proper append buffer
void editorSave() {
  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL, NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
  }

  int len;
  char *buf = editorRowsToString(&len);

  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        E.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }

  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/***
######## #### ##    ## ########
##        ##  ###   ## ##     ##
##        ##  ####  ## ##     ##
######    ##  ## ## ## ##     ##
##        ##  ##  #### ##     ##
##        ##  ##   ### ##     ##
##       #### ##    ## ########
 ***/

struct findCallbackState {
  int last_match;
  int last_match_x;
  int direction;
};

// FIXME search backwards still searches forwards on the line
// TODO remember first result for each char in the buffer, proper incremental
void editorFindCallback(char *query, int querylen, int key, void *state_param) {
  struct findCallbackState *state = (struct findCallbackState *) state_param;

  if (key == '\r' || key == '\x1b') {
    state->last_match = -1;
    state->direction = 1;
    return;
  } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    state->direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    state->direction = -1;
  } else {
    state->last_match = -1;
    state->direction = 1;
  }

  int current = state->last_match;
  if (state->last_match == -1) {
    state->direction = 1;
  } else { // TODO don't duplicate this code (but make it fast!)
    erow *row = &E.row[current];
    char *match = strstr(row->render + state->last_match_x + 1, query);
    if (match) {
      state->last_match = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render) + querylen; // go to end of match
      state->last_match_x = E.cx;
      E.rowoff = E.numrows;
      editorScroll();
      return; // eek!
    }
  }
  int i;
  for (i = 0; i < E.numrows; i++) {
    current += state->direction;
    if (current == -1) current = E.numrows - 1;
    else if (current == E.numrows) current = 0;
    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      state->last_match = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render) + querylen; // go to end of match
      state->last_match_x = E.cx;
      E.rowoff = E.numrows;
      editorScroll();
      break;
    }
  }

  // int current = state->last_match;
  // if(state->direction == 1) {
  //   erow *row = &E.row[current];
  //   char *match = strstr(row->chars, query);
  //   if (match) {
  //     state->last_match = current;
  //     E.cy = current;
  //     E.cx = match - row->chars;
  //     state->last_match_start_x = E.cx;
  //     state->last_match_end_x = E.cx + strlen(query);
  //     E.rowoff = E.numrows;
  //     editorScroll();
  //     return;
  //   }
  // }

}

void editorFind() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_rx = E.rx;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;

  struct findCallbackState *state = malloc(sizeof(struct findCallbackState));

  state->last_match = -1;
  state->last_match_x = -1;
  state->direction = 1;

  char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",
                             editorFindCallback, state);

  if (state) {
    free(state);
  }

  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.rx = saved_rx;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}

/***
   ###    ########  ########  ######## ##    ## ########
  ## ##   ##     ## ##     ## ##       ###   ## ##     ##
 ##   ##  ##     ## ##     ## ##       ####  ## ##     ##
##     ## ########  ########  ######   ## ## ## ##     ##
######### ##        ##        ##       ##  #### ##     ##
##     ## ##        ##        ##       ##   ### ##     ##
##     ## ##        ##        ######## ##    ## ########
 ***/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}
void abFree(struct abuf *ab) {
  free(ab->b);
}

/***
 #######  ##     ## ######## ########  ##     ## ########
##     ## ##     ##    ##    ##     ## ##     ##    ##
##     ## ##     ##    ##    ##     ## ##     ##    ##
##     ## ##     ##    ##    ########  ##     ##    ##
##     ## ##     ##    ##    ##        ##     ##    ##
##     ## ##     ##    ##    ##        ##     ##    ##
 #######   #######     ##    ##         #######     ##
 ***/

// FIXME why the hell is this in scroll and not movecursor?
void editorScroll() {
  if (E.cy < E.numrows) {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  } else {
    E.rx = 0;
  }

  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= (E.rowoff + E.screenrows) - 1) {
    E.rowoff = (E.cy - E.screenrows) + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= (E.coloff + E.screencols) - 1) {
    E.coloff = (E.rx - E.screencols) + 1;
  }
}


void editorDrawRows(struct abuf *ab) {
  int y = 0; // row counter

  // Pointer to first row after offset
  struct erow *row = E.row + E.rowoff;

  // Calculate row to stop printing at
  int maxrows = E.numrows - E.rowoff;
  maxrows = maxrows < E.screenrows ? maxrows : E.screenrows;

  // Print
  for (; y < maxrows; ++y, ++row) { // Increment row pointer
    int len = row->rsize - E.coloff;
    if (len < 0) len = 0;
    if (len > E.screencols) len = E.screencols;

    abAppend(ab, row->render + E.coloff, len); // Offset char pointer
    abAppend(ab, "\x1b[K\r\n", 5);
  }

  // For any extra rows print a ~
  for (; y < E.screenrows; ++y) {
    abAppend(ab, "~\x1b[K\r\n", 6); // tilde clearline carriagereturn linefeed
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  abAppend(ab, "\x1b[7m", 4);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    E.filename ? E.filename : "[No Name]", E.numrows,
    E.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
    E.cy + 1, E.numrows);
  if (len > E.screencols) len = E.screencols;
  abAppend(ab, status, len);

  // TODO make status bar more efficient
  while (len < E.screencols) {
    if (E.screencols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2); // TODO should do this in same line
}

void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmsg);
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6); // Hide cursor to prevent flicker
  abAppend(&ab, "\x1b[2J", 4); // Clear whole screen
  abAppend(&ab, "\x1b[H", 3); // Reset cursor position

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
                                            (E.rx - E.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6); // Show cursor

  write(STDOUT_FILENO, ab.b, ab.len);

  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
}

/***
#### ##    ## ########  ##     ## ########
 ##  ###   ## ##     ## ##     ##    ##
 ##  ####  ## ##     ## ##     ##    ##
 ##  ## ## ## ########  ##     ##    ##
 ##  ##  #### ##        ##     ##    ##
 ##  ##   ### ##        ##     ##    ##
#### ##    ## ##         #######     ##
 ***/

char *editorPrompt(char *prompt, void (*callback)(char *, int, int, void *), void *state) {
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
        E.cx = 0;
      }
      break;
    case ARROW_UP:
      if (E.cy != 0) {
        E.cy--;
      }
      break;
    case ARROW_DOWN:
      if (E.cy < E.numrows) {
        E.cy++;
      }
      break;
  }

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

/***
##       ##     ##    ###
##       ##     ##   ## ##
##       ##     ##  ##   ##
##       ##     ## ##     ##
##       ##     ## #########
##       ##     ## ##     ##
########  #######  ##     ##
 ***/

int luaEventKeypress(int c) {
  lua_getglobal(L, "key");
  lua_pushinteger(L, c);

  if (lua_pcall(L, 1, 1, 0) != 0) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1)); // tell us what mistake we made
    return 0;
  }

  if (!lua_isnumber(L, -1)) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1)); // tell us what mistake we made
    return 0;
  }

  int ret = lua_tointeger(L, -1);
  lua_pop(L, 1);
  return ret;
}

int extSetStatusMessage(lua_State *L) {
  const char *message = luaL_checkstring(L, 1);

  editorSetStatusMessage(message);

  return 0;
}

// TODO consider making this non-global
int luaInit() {
  L = luaL_newstate(); // open Lua
  if (!L) {
    return -1; // Checks that Lua started up
  }

  luaL_openlibs(L); // load Lua libraries

  lua_createtable(L, 0, 1);

  lua_pushstring(L, "status");
  lua_pushcfunction(L, extSetStatusMessage);
  lua_settable(L, -3);

  lua_setglobal(L, "le");

  return 0;
}

int luaExecuteScript(const char *filename) {
  int status = luaL_loadfile(L, filename);  // load Lua script
  if (status != 0) {
    fprintf(stderr, "Couldn't open file %s\n", filename); // tell us what mistake we made
    return 0;
  }
  int ret = lua_pcall(L, 0, 0, 0); // tell Lua to run the script
  if (ret != 0) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1)); // tell us what mistake we made
    return 1;
  }
  return 0;
}

/***
#### ##    ## #### ########
 ##  ###   ##  ##     ##
 ##  ####  ##  ##     ##
 ##  ## ## ##  ##     ##
 ##  ##  ####  ##     ##
 ##  ##   ###  ##     ##
#### ##    ## ####    ##
 ***/

void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  E.screenrows -= 2;
}

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