#ifndef __LE_FIND_H__
#define __LE_FIND_H__

struct epos {
  int x;
  int y;
};

struct findCallbackState {
  int last_match;
  int last_match_x;
  int direction;
};

void editorFindCallback(char *query, int querylen, int key, void *state_param);

void editorFind();

struct epos editorFindString(const char *query);

#endif
