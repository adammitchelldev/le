/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <terminal.h>
#include <row.h>
#include <operations.h>
#include <file.h>
#include <output.h>
#include <editor.h>

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
