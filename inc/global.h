#ifndef __LE_GLOBAL_H__
#define __LE_GLOBAL_H__

#include "row.h"

#define LE_VERSION "0.0.1"
#define LE_TAB_STOP 2
#define LE_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

#endif
