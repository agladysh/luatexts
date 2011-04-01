/*
* luatexts.c: Trivial Lua human-readable binary-safe serialization library
*             See copyright information in file COPYRIGHT.
*/

#if defined (__cplusplus)
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>

#if defined (__cplusplus)
}
#endif

#if 0
  #define SPAM(a) printf a
#else
  #define SPAM(a) (void)0
#endif

#define LUATEXTS_VERSION     "luatexts 0.1"
#define LUATEXTS_COPYRIGHT   "Copyright (C) 2011, luatexts authors"
#define LUATEXTS_DESCRIPTION \
    "Trivial Lua human-readable binary-safe serialization library"

static int lload(lua_State * L)
{
  return luaL_error(L, "TODO: Implement!");
}

/* Lua module API */
static const struct luaL_reg R[] =
{
  { "load", lload },

  { NULL, NULL }
};

#ifdef __cplusplus
extern "C" {
#endif

LUALIB_API int luaopen_luatexts(lua_State * L)
{
  /*
  * Register module
  */
  luaL_register(L, "luatexts", R);

  /*
  * Register module information
  */
  lua_pushliteral(L, LUATEXTS_VERSION);
  lua_setfield(L, -2, "_VERSION");

  lua_pushliteral(L, LUATEXTS_COPYRIGHT);
  lua_setfield(L, -2, "_COPYRIGHT");

  lua_pushliteral(L, LUATEXTS_DESCRIPTION);
  lua_setfield(L, -2, "_DESCRIPTION");

  return 1;
}

#ifdef __cplusplus
}
#endif
