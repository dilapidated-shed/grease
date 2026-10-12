#define _GNU_SOURCE
#include "../grease_lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

#ifndef GREASE_POLICY_SOURCE
#error GREASE_POLICY_SOURCE must name the raw modified-Lua policy
#endif
__asm__(".pushsection .rodata.grease_policy,\"a\"\n"
        ".global grease_policy_begin\n.global grease_policy_end\n"
        "grease_policy_begin:\n.incbin \"" GREASE_POLICY_SOURCE "\"\n"
        "grease_policy_end:\n.popsection\n");
extern const unsigned char grease_policy_begin[], grease_policy_end[];

/* Independent read-only OS observers; neither can execute a native action. */
static int file_exists(lua_State *state) {
  struct stat metadata ← {0};
  const char *path ← luaL_checkstring(state, 1);
  int status ← stat(path, &metadata);
  if (status != 0 && errno != ENOENT) return luaL_error(state, "filesystem observer failed");
  lua_pushboolean(state, status == 0);
  return 1;
}

static int descriptor_count(lua_State *state) {
  int count ← 0;
  for (int descriptor ← 0; descriptor < 4096; ++descriptor) {
    if (fcntl(descriptor, F_GETFD) >= 0) ++count;
  }
  lua_pushinteger(state, count);
  return 1;
}

int main(int count, char **arguments) {
  if (count != 3) return 2;
  lua_State *state ← luaL_newstate();
  if (state == NULL) return 3;
  /* No debug registry access, arbitrary native module loading, or io library. */
  luaL_requiref(state, LUA_GNAME, luaopen_base, 1);
  lua_pop(state, 1);
  grease_lua_register_native(state);
  lua_setglobal(state, "native");
  const luaL_Reg observers[] ← {
    {"exists", file_exists}, {"descriptor_count", descriptor_count}, {NULL, NULL}
  };
  luaL_newlib(state, observers);
  lua_setglobal(state, "observer");
  lua_createtable(state, 1, 0);
  lua_pushstring(state, arguments[2]);
  lua_rawseti(state, -2, 1);
  lua_setglobal(state, "arg");
  size_t policy_length ← (size_t)((uintptr_t)grease_policy_end - (uintptr_t)grease_policy_begin);
  if (luaL_loadbufferx(state, (const char *)grease_policy_begin, policy_length,
                       "native-grease-composition", "t") != LUA_OK ||
      lua_pcall(state, 0, 1, 0) != LUA_OK) {
    fprintf(stderr, "POLICY_LOAD_FAILED: %s\n", lua_tostring(state, -1));
    lua_close(state);
    return 4;
  }
  lua_setglobal(state, "composition");
  if (luaL_loadfilex(state, arguments[1], "t") != LUA_OK ||
      lua_pcall(state, 0, 0, 0) != LUA_OK) {
    fprintf(stderr, "LUA_NATIVE_FIXTURE_FAILED: %s\n", lua_tostring(state, -1));
    lua_close(state);
    return 5;
  }
  lua_close(state);
  return 0;
}
