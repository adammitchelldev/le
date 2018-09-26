/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <find.h>
#include <editor.h>
#include <output.h>
#include <input.h>

#include <stdlib.h>
#include <string.h>

#include <global.h>

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

struct epos editorFindString(const char *query) {
  struct epos p = {0,0};
  int current;
  for (current = 0; current < E.numrows; current++) {
    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      p.y = current;
      p.x = editorRowRxToCx(row, match - row->render);
      return p;
    }
  }
  return p;
}
