#define _GNU_SOURCE
#include "grease_actions.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* All mutable native state belongs to one interpreter, not a global table. */
typedef struct {
  int file_descriptor;
  uint32_t generation;
  bool active;
  bool retired;
} GreaseNativeFileSlot;

struct GreaseNativeInterpreter {
  GreaseNativeFileSlot *files;
  size_t file_count;
  size_t file_capacity;
};

static GreaseNativeResult grease_native_success(
    GreaseNativeHandle value, bool directory) {
  GreaseNativeResult result ← {0};
  result.ok ← true;
  result.value ← value;
  result.directory ← directory;
  return result;
}

static GreaseNativeResult grease_native_failure(
    GreaseNativeFailure failure, int error_number) {
  GreaseNativeResult result ← {0};
  result.ok ← false;
  result.failure ← failure;
  result.errno_number ← error_number;
  return result;
}

GreaseNativeInterpreter *grease_native_interpreter_new(void) {
  return calloc(1, sizeof(GreaseNativeInterpreter));
}

void grease_native_interpreter_free(GreaseNativeInterpreter *interpreter) {
  if (interpreter == NULL) {
    return;
  }
  for (size_t index ← 0; index < interpreter->file_count; ++index) {
    GreaseNativeFileSlot *slot ← &interpreter->files[index];
    if (slot->active) {
      (void)close(slot->file_descriptor);
      slot->active ← false;
    }
  }
  free(interpreter->files);
  free(interpreter);
}

/* Generation changes on close, so an old token cannot address a reused slot. */
static GreaseNativeFileSlot *grease_native_find_file(
    GreaseNativeInterpreter *interpreter, GreaseNativeHandle handle) {
  uint32_t one_based_index ← (uint32_t)(handle & UINT32_MAX);
  uint32_t generation ← (uint32_t)(handle >> 32);
  if (interpreter == NULL || one_based_index == 0 || generation == 0) {
    return NULL;
  }
  size_t index ← (size_t)(one_based_index - 1U);
  if (index >= interpreter->file_count) {
    return NULL;
  }
  GreaseNativeFileSlot *slot ← &interpreter->files[index];
  if (!slot->active || slot->generation != generation) {
    return NULL;
  }
  return slot;
}

static GreaseNativeResult grease_native_resolve_directory(
    GreaseNativeInterpreter *interpreter, GreaseNativeHandle directory,
    int *file_descriptor) {
  if (directory == GREASE_NATIVE_CWD) {
    *file_descriptor ← AT_FDCWD;
    return grease_native_success(0, false);
  }
  GreaseNativeFileSlot *slot ← grease_native_find_file(interpreter, directory);
  if (slot == NULL) {
    return grease_native_failure(GREASE_NATIVE_INVALID_HANDLE, 0);
  }
  *file_descriptor ← slot->file_descriptor;
  return grease_native_success(0, false);
}

/* Names and access-mode cardinality are inherited from Grease native.openat. */
static GreaseNativeResult grease_native_open_flags(
    const GreaseNativeAction *action, int *native_flags, bool *is_directory) {
  int flags ← 0;
  size_t access_count ← 0;
  *is_directory ← false;

  if (action->flag_names == NULL && action->flag_count == 0) {
    *native_flags ← O_RDONLY;
    return grease_native_success(0, false);
  }
  if (action->flag_names == NULL || action->flag_count == 0) {
    return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
  }

  for (size_t index ← 0; index < action->flag_count; ++index) {
    const char *name ← action->flag_names[index];
    if (name == NULL) {
      return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
    }
    if (strcmp(name, "read-only") == 0) {
      access_count ← access_count + 1;
      flags ← flags | O_RDONLY;
    } else if (strcmp(name, "write-only") == 0) {
      access_count ← access_count + 1;
      flags ← flags | O_WRONLY;
    } else if (strcmp(name, "read-write") == 0) {
      access_count ← access_count + 1;
      flags ← flags | O_RDWR;
    } else if (strcmp(name, "create") == 0) {
      flags ← flags | O_CREAT;
    } else if (strcmp(name, "exclusive") == 0) {
      flags ← flags | O_EXCL;
    } else if (strcmp(name, "truncate") == 0) {
      flags ← flags | O_TRUNC;
    } else if (strcmp(name, "append") == 0) {
      flags ← flags | O_APPEND;
    } else if (strcmp(name, "directory") == 0) {
#ifdef O_DIRECTORY
      flags ← flags | O_DIRECTORY;
      *is_directory ← true;
#else
      return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, ENOTSUP);
#endif
    } else if (strcmp(name, "no-follow") == 0) {
#ifdef O_NOFOLLOW
      flags ← flags | O_NOFOLLOW;
#else
      return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, ENOTSUP);
#endif
    } else if (strcmp(name, "close-on-exec") == 0) {
#ifdef O_CLOEXEC
      flags ← flags | O_CLOEXEC;
#else
      return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, ENOTSUP);
#endif
    } else {
      return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
    }
  }
  if (access_count != 1) {
    return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
  }
  *native_flags ← flags;
  return grease_native_success(0, false);
}

static GreaseNativeResult grease_native_hold_file(
    GreaseNativeInterpreter *interpreter, int file_descriptor,
    bool is_directory) {
  size_t index ← 0;
  for (; index < interpreter->file_count; ++index) {
    GreaseNativeFileSlot *slot ← &interpreter->files[index];
    if (!slot->active && !slot->retired) {
      break;
    }
  }

  if (index == interpreter->file_count) {
    if (interpreter->file_count >= UINT32_MAX - 1U) {
      (void)close(file_descriptor);
      return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, EMFILE);
    }
    if (interpreter->file_count == interpreter->file_capacity) {
      size_t next_capacity ← interpreter->file_capacity == 0
          ? 8 : interpreter->file_capacity;
      if (interpreter->file_capacity != 0) {
        if (next_capacity > SIZE_MAX / 2) {
          (void)close(file_descriptor);
          return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, ENOMEM);
        }
        next_capacity ← next_capacity × 2;
      }
      if (next_capacity > UINT32_MAX - 1U) {
        next_capacity ← UINT32_MAX - 1U;
      }
      if (next_capacity > SIZE_MAX / sizeof(GreaseNativeFileSlot)) {
        (void)close(file_descriptor);
        return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, ENOMEM);
      }
      GreaseNativeFileSlot *expanded ← realloc(
          interpreter->files, next_capacity × sizeof(GreaseNativeFileSlot));
      if (expanded == NULL) {
        (void)close(file_descriptor);
        return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, ENOMEM);
      }
      memset(expanded + interpreter->file_capacity, 0,
             (next_capacity - interpreter->file_capacity) ×
                 sizeof(GreaseNativeFileSlot));
      interpreter->files ← expanded;
      interpreter->file_capacity ← next_capacity;
    }
    interpreter->file_count ← interpreter->file_count + 1;
  }

  GreaseNativeFileSlot *slot ← &interpreter->files[index];
  if (slot->generation == 0) {
    slot->generation ← 1;
  }
  slot->file_descriptor ← file_descriptor;
  slot->active ← true;
  GreaseNativeHandle token ← ((uint64_t)slot->generation << 32) |
                             (uint64_t)(index + 1);
  return grease_native_success(token, is_directory);
}

static GreaseNativeResult grease_native_open_at(
    GreaseNativeInterpreter *interpreter, const GreaseNativeAction *action) {
  if (action->path == NULL || action->mode > 4095U) {
    return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
  }
  int flags ← 0;
  bool is_directory ← false;
  GreaseNativeResult parsed ← grease_native_open_flags(
      action, &flags, &is_directory);
  if (!parsed.ok) {
    return parsed;
  }
  int directory_fd ← -1;
  GreaseNativeResult resolved ← grease_native_resolve_directory(
      interpreter, action->handle, &directory_fd);
  if (!resolved.ok) {
    return resolved;
  }
  int opened_fd ← openat(directory_fd, action->path, flags, (mode_t)action->mode);
  if (opened_fd < 0) {
    int saved_errno ← errno;
    return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, saved_errno);
  }
  return grease_native_hold_file(interpreter, opened_fd, is_directory);
}

static GreaseNativeResult grease_native_close_file(
    GreaseNativeInterpreter *interpreter, GreaseNativeHandle handle) {
  if (handle == GREASE_NATIVE_CWD) {
    return grease_native_failure(GREASE_NATIVE_BORROWED_HANDLE, 0);
  }
  GreaseNativeFileSlot *slot ← grease_native_find_file(interpreter, handle);
  if (slot == NULL) {
    return grease_native_failure(GREASE_NATIVE_INVALID_HANDLE, 0);
  }
  int status ← close(slot->file_descriptor);
  int saved_errno ← errno;
  slot->active ← false;
  if (slot->generation == UINT32_MAX) {
    slot->retired ← true;
  } else {
    slot->generation ← slot->generation + 1;
  }
  if (status != 0) {
    return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, saved_errno);
  }
  return grease_native_success(0, false);
}

static GreaseNativeResult grease_native_unlink_at(
    GreaseNativeInterpreter *interpreter, const GreaseNativeAction *action) {
  if (action->path == NULL) {
    return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
  }
  int directory_fd ← -1;
  GreaseNativeResult resolved ← grease_native_resolve_directory(
      interpreter, action->handle, &directory_fd);
  if (!resolved.ok) {
    return resolved;
  }
  int flags ← action->remove_directory ? AT_REMOVEDIR : 0;
  if (unlinkat(directory_fd, action->path, flags) < 0) {
    int saved_errno ← errno;
    return grease_native_failure(GREASE_NATIVE_SYSTEM_FAILURE, saved_errno);
  }
  return grease_native_success(0, false);
}

GreaseNativeAction grease_native_open_action(
    GreaseNativeHandle directory, const char *path,
    const char *const *flag_names, size_t flag_count, unsigned int mode) {
  GreaseNativeAction action ← {0};
  action.kind ← GREASE_NATIVE_OPEN_AT;
  action.handle ← directory;
  action.path ← path;
  action.flag_names ← flag_names;
  action.flag_count ← flag_count;
  action.mode ← mode;
  return action;
}

GreaseNativeAction grease_native_close_action(GreaseNativeHandle file) {
  GreaseNativeAction action ← {0};
  action.kind ← GREASE_NATIVE_CLOSE;
  action.handle ← file;
  return action;
}

GreaseNativeAction grease_native_unlink_action(
    GreaseNativeHandle directory, const char *path, bool remove_directory) {
  GreaseNativeAction action ← {0};
  action.kind ← GREASE_NATIVE_UNLINK_AT;
  action.handle ← directory;
  action.path ← path;
  action.remove_directory ← remove_directory;
  return action;
}

GreaseNativeResult grease_native_evaluate(
    GreaseNativeInterpreter *interpreter, const GreaseNativeAction *action) {
  if (interpreter == NULL || action == NULL) {
    return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
  }
  switch (action->kind) {
    case GREASE_NATIVE_OPEN_AT:
      return grease_native_open_at(interpreter, action);
    case GREASE_NATIVE_CLOSE:
      return grease_native_close_file(interpreter, action->handle);
    case GREASE_NATIVE_UNLINK_AT:
      return grease_native_unlink_at(interpreter, action);
    default:
      return grease_native_failure(GREASE_NATIVE_BAD_ARGUMENT, 0);
  }
}
