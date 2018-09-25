/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <editor.h>
#include <output.h>
#include <input.h>
#include <events.h>
#include <find.h>

#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

int luaEventKeypress(int c) {
  lua_getglobal(L, "key");
  lua_pushinteger(L, c);

  if (lua_pcall(L, 1, 1, 0) != 0) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1)); // tell us what mistake we made
    editorSetStatusMessage("%s", lua_tostring(L, -1)); // tell us what mistake we made
    return 0;
  }

  if (!lua_isnumber(L, -1)) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1)); // tell us what mistake we made
    editorSetStatusMessage("%s", lua_tostring(L, -1)); // tell us what mistake we made
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

static void promptCallback(char *buf, int buflen, int key, void *state) {
  lua_State *L = (lua_State *) state;

  lua_pushvalue(L, 2); // push the callback function
  lua_pushlstring(L, buf, buflen); // the current string
  lua_pushinteger(L, key); // the last keycode

  int n = lua_gettop(L);
  int i;
  for(i = 3; i <= n; i++) {
    lua_pushvalue(L, i); // any additional user args passed to prompt()
  }

  lua_call(L, n, 0); // n + 2 - 2
}

int extPrompt(lua_State *L) {
  const char *prompt = luaL_checkstring(L, 1);
  char *result;

  if(lua_isfunction(L, 2)) {
    result = editorPrompt(prompt, promptCallback, L);
  } else {
    result = editorPrompt(prompt, NULL, L);
  }

  if(result) {
    lua_pushstring(L, result);
    return 1;
  } else {
    return 0;
  }
}

/* Find the next occurance of a string in the buffer
 * returns the row number and column
 */
int extFind(lua_State *L) {
  const char* query = luaL_checkstring(L, 1);

  struct epos result = editorFindString(query);

  lua_pushinteger(L, result.x);
  lua_pushinteger(L, result.y);

  return 2;
}

int extGo(lua_State *L) {
  E.cx = luaL_checkinteger(L, 1);
  E.cy = luaL_checkinteger(L, 2);
  editorScroll();
  return 0;
}

// TODO consider making this non-global
int luaInit() {
  L = luaL_newstate(); // open Lua
  if (!L) {
    return -1; // Checks that Lua started up
  }

  luaL_openlibs(L); // load Lua libraries

  lua_createtable(L, 0, 4);

  lua_pushstring(L, "status");
  lua_pushcfunction(L, extSetStatusMessage);
  lua_settable(L, -3);

  lua_pushstring(L, "prompt");
  lua_pushcfunction(L, extPrompt);
  lua_settable(L, -3);

  lua_pushstring(L, "find");
  lua_pushcfunction(L, extFind);
  lua_settable(L, -3);

  lua_pushstring(L, "go");
  lua_pushcfunction(L, extGo);
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
