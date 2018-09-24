/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <output.h>
#include <events.h>

#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

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
