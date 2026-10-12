#include "grease_lua.h"
#include "grease_actions.h"
#include "lauxlib.h"
#include <errno.h>
#include <string.h>

#define INTERPRETER_TYPE "GreaseNative.Interpreter"
#define HANDLE_TYPE "GreaseNative.Handle"
#define ACTION_TYPE "GreaseNative.Action"
#define MAXIMUM_FLAG_NAMES 64U

typedef struct {
  GreaseNativeInterpreter *interpreter;
} GreaseLuaInterpreter;

typedef struct {
  GreaseLuaInterpreter *owner;
  GreaseNativeHandle handle;
} GreaseLuaHandle;

typedef struct {
  GreaseLuaInterpreter *owner;
  GreaseNativeAction action;
  const char *flag_names[];
} GreaseLuaAction;

static GreaseLuaInterpreter *checked_interpreter(lua_State *state, int index) {
  GreaseLuaInterpreter *owner ← luaL_checkudata(state, index, INTERPRETER_TYPE);
  luaL_argcheck(state, owner->interpreter != NULL, index, "inactive interpreter");
  return owner;
}

static GreaseLuaHandle *checked_handle(
    lua_State *state, int index, GreaseLuaInterpreter *owner) {
  GreaseLuaHandle *handle ← luaL_checkudata(state, index, HANDLE_TYPE);
  luaL_argcheck(state, handle->owner == owner, index,
                "handle belongs to another interpreter");
  return handle;
}

static const char *checked_text(lua_State *state, int index) {
  luaL_checktype(state, index, LUA_TSTRING);
  size_t length ← 0;
  const char *text ← lua_tolstring(state, index, &length);
  luaL_argcheck(state, memchr(text, 0, length) == NULL, index,
                "embedded zero byte in native text");
  return text;
}

static void retain_owner(lua_State *state, int userdata_index) {
  lua_pushvalue(state, 1);
  lua_setiuservalue(state, userdata_index, 1);
}

static int dispose_interpreter(lua_State *state) {
  GreaseLuaInterpreter *owner ← luaL_checkudata(state, 1, INTERPRETER_TYPE);
  grease_native_interpreter_free(owner->interpreter);
  owner->interpreter ← NULL;
  return 0;
}

static int new_interpreter(lua_State *state) {
  luaL_argcheck(state, lua_gettop(state) == 0, 1, "no arguments expected");
  GreaseLuaInterpreter *owner ← lua_newuserdatauv(state, sizeof *owner, 0);
  owner->interpreter ← NULL;
  luaL_setmetatable(state, INTERPRETER_TYPE);
  owner->interpreter ← grease_native_interpreter_new();
  if (owner->interpreter == NULL) return luaL_error(state, "native interpreter allocation failed");
  return 1;
}

static void push_handle(
    lua_State *state, GreaseLuaInterpreter *owner, GreaseNativeHandle token) {
  GreaseLuaHandle *handle ← lua_newuserdatauv(state, sizeof *handle, 1);
  handle->owner ← owner;
  handle->handle ← token;
  luaL_setmetatable(state, HANDLE_TYPE);
  retain_owner(state, lua_gettop(state));
}

static int working_directory(lua_State *state) {
  luaL_argcheck(state, lua_gettop(state) == 1, 1, "one interpreter expected");
  GreaseLuaInterpreter *owner ← checked_interpreter(state, 1);
  push_handle(state, owner, GREASE_NATIVE_CWD);
  return 1;
}

static GreaseLuaAction *new_action(
    lua_State *state, GreaseLuaInterpreter *owner, size_t flag_count) {
  GreaseLuaAction *action ← lua_newuserdatauv(
      state, sizeof *action + flag_count × sizeof(const char *), 2);
  memset(action, 0, sizeof *action);
  action->owner ← owner;
  luaL_setmetatable(state, ACTION_TYPE);
  retain_owner(state, lua_gettop(state));
  return action;
}

static int open_at_action(lua_State *state) {
  int argument_count ← lua_gettop(state);
  luaL_argcheck(state, argument_count >= 3 && argument_count <= 5, 1,
                "interpreter, directory, path, optional flags and mode expected");
  GreaseLuaInterpreter *owner ← checked_interpreter(state, 1);
  GreaseLuaHandle *directory ← checked_handle(state, 2, owner);
  const char *path ← checked_text(state, 3);
  bool has_flags ← !lua_isnoneornil(state, 4);
  size_t flag_count ← 0;
  if (has_flags) {
    luaL_checktype(state, 4, LUA_TTABLE);
    flag_count ← lua_rawlen(state, 4);
    luaL_argcheck(state, flag_count <= MAXIMUM_FLAG_NAMES, 4, "too many native flags");
    size_t entries ← 0;
    lua_pushnil(state);
    while (lua_next(state, 4) != 0) {
      luaL_argcheck(state, lua_isinteger(state, -2) &&
                    lua_tointeger(state, -2) >= 1 &&
                    (uint64_t)lua_tointeger(state, -2) <= flag_count,
                    4, "flags must form a dense list");
      ++entries;
      lua_pop(state, 1);
    }
    luaL_argcheck(state, entries == flag_count, 4, "flags must form a dense list");
  }
  if (!lua_isnoneornil(state, 5)) {
    luaL_argcheck(state, lua_isinteger(state, 5), 5, "integer permission bits expected");
  }
  lua_Integer mode ← luaL_optinteger(state, 5, 0666);
  luaL_argcheck(state, mode >= 0 && mode <= 4095, 5, "permission bits out of range");

  GreaseLuaAction *action ← new_action(state, owner, flag_count);
  int action_index ← lua_gettop(state);
  lua_createtable(state, (int)flag_count + 1, 0);
  lua_pushvalue(state, 3);
  lua_rawseti(state, -2, 1);
  for (size_t index ← 0; index < flag_count; ++index) {
    lua_rawgeti(state, 4, (lua_Integer)index + 1);
    action->flag_names[index] ← checked_text(state, -1);
    lua_rawseti(state, -2, (lua_Integer)index + 2);
  }
  lua_setiuservalue(state, action_index, 2);
  action->action ← grease_native_open_action(
      directory->handle, path, has_flags ? action->flag_names : NULL,
      flag_count, (unsigned int)mode);
  return 1;
}

static int close_action(lua_State *state) {
  luaL_argcheck(state, lua_gettop(state) == 2, 1, "interpreter and handle expected");
  GreaseLuaInterpreter *owner ← checked_interpreter(state, 1);
  GreaseLuaHandle *file ← checked_handle(state, 2, owner);
  GreaseLuaAction *action ← new_action(state, owner, 0);
  action->action ← grease_native_close_action(file->handle);
  return 1;
}

static int unlink_at_action(lua_State *state) {
  int argument_count ← lua_gettop(state);
  luaL_argcheck(state, argument_count >= 3 && argument_count <= 4, 1,
                "interpreter, directory, path and optional remove-directory expected");
  GreaseLuaInterpreter *owner ← checked_interpreter(state, 1);
  GreaseLuaHandle *directory ← checked_handle(state, 2, owner);
  const char *path ← checked_text(state, 3);
  bool remove_directory ← false;
  if (!lua_isnoneornil(state, 4)) {
    luaL_checktype(state, 4, LUA_TBOOLEAN);
    remove_directory ← lua_toboolean(state, 4);
  }
  GreaseLuaAction *action ← new_action(state, owner, 0);
  int action_index ← lua_gettop(state);
  lua_pushvalue(state, 3);
  lua_setiuservalue(state, action_index, 2);
  action->action ← grease_native_unlink_action(directory->handle, path, remove_directory);
  return 1;
}

static const char *failure_kind(GreaseNativeFailure failure) {
  switch (failure) {
    case GREASE_NATIVE_BAD_ARGUMENT: return "bad_argument";
    case GREASE_NATIVE_INVALID_HANDLE: return "invalid_handle";
    case GREASE_NATIVE_BORROWED_HANDLE: return "borrowed_handle";
    case GREASE_NATIVE_SYSTEM_FAILURE: return "system_failure";
    default: return "none";
  }
}

static const char *failure_message(GreaseNativeResult result) {
  switch (result.failure) {
    case GREASE_NATIVE_BAD_ARGUMENT: return "Invalid native action argument";
    case GREASE_NATIVE_INVALID_HANDLE: return "Invalid or closed native resource";
    case GREASE_NATIVE_BORROWED_HANDLE: return "Borrowed working directory cannot be closed";
    case GREASE_NATIVE_SYSTEM_FAILURE: return strerror(result.errno_number);
    default: return "No native failure";
  }
}

static int evaluate_action(lua_State *state) {
  luaL_argcheck(state, lua_gettop(state) == 2, 1, "interpreter and action expected");
  GreaseLuaInterpreter *owner ← checked_interpreter(state, 1);
  GreaseLuaAction *action ← luaL_checkudata(state, 2, ACTION_TYPE);
  luaL_argcheck(state, action->owner == owner, 2,
                "action belongs to another interpreter");
  GreaseNativeResult result ← grease_native_evaluate(owner->interpreter, &action->action);
  lua_createtable(state, 0, 4);
  lua_pushboolean(state, result.ok);
  lua_setfield(state, -2, "ok");
  if (result.ok && action->action.kind == GREASE_NATIVE_OPEN_AT) {
    push_handle(state, owner, result.value);
    lua_setfield(state, -2, "value");
    lua_pushboolean(state, result.directory);
    lua_setfield(state, -2, "directory");
  }
  if (!result.ok) {
    lua_createtable(state, 0, 3);
    lua_pushstring(state, failure_kind(result.failure));
    lua_setfield(state, -2, "kind");
    lua_pushinteger(state, result.errno_number);
    lua_setfield(state, -2, "number");
    lua_pushstring(state, failure_message(result));
    lua_setfield(state, -2, "message");
    lua_setfield(state, -2, "error");
  }
  return 1;
}

int grease_lua_register_native(lua_State *state) {
  luaL_newmetatable(state, INTERPRETER_TYPE);
  lua_pushcfunction(state, dispose_interpreter);
  lua_setfield(state, -2, "__gc");
  lua_pushboolean(state, 0);
  lua_setfield(state, -2, "__metatable");
  lua_pop(state, 1);
  luaL_newmetatable(state, HANDLE_TYPE);
  lua_pushboolean(state, 0);
  lua_setfield(state, -2, "__metatable");
  lua_pop(state, 1);
  luaL_newmetatable(state, ACTION_TYPE);
  lua_pushboolean(state, 0);
  lua_setfield(state, -2, "__metatable");
  lua_pop(state, 1);
  const luaL_Reg methods[] ← {
    {"new_interpreter", new_interpreter},
    {"working_directory", working_directory},
    {"open_at_action", open_at_action},
    {"close_action", close_action},
    {"unlink_at_action", unlink_at_action},
    {"evaluate", evaluate_action},
    {NULL, NULL}
  };
  luaL_newlib(state, methods);
  return 1;
}
