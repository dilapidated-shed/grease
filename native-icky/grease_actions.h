#ifndef GREASE_ICKY_NATIVE_ACTIONS_H
#define GREASE_ICKY_NATIVE_ACTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* This is a C-side token. Language consumers must wrap it as opaque userdata. */
typedef uint64_t GreaseNativeHandle;
#define GREASE_NATIVE_CWD UINT64_C(0)

typedef struct GreaseNativeInterpreter GreaseNativeInterpreter;

typedef enum {
  GREASE_NATIVE_OPEN_AT,
  GREASE_NATIVE_CLOSE,
  GREASE_NATIVE_UNLINK_AT
} GreaseNativeActionKind;

typedef enum {
  GREASE_NATIVE_NO_FAILURE,
  GREASE_NATIVE_BAD_ARGUMENT,
  GREASE_NATIVE_INVALID_HANDLE,
  GREASE_NATIVE_BORROWED_HANDLE,
  GREASE_NATIVE_SYSTEM_FAILURE
} GreaseNativeFailure;

typedef struct {
  bool ok;
  GreaseNativeFailure failure;
  int errno_number;
  GreaseNativeHandle value;
  bool directory;
} GreaseNativeResult;

/*
 * An action is a value: construction performs no OS work. Evaluation is
 * synchronous. Its path and flag names must remain live through evaluation.
 */
typedef struct {
  GreaseNativeActionKind kind;
  GreaseNativeHandle handle;
  const char *path;
  const char *const *flag_names;
  size_t flag_count;
  unsigned int mode;
  bool remove_directory;
} GreaseNativeAction;

GreaseNativeInterpreter *grease_native_interpreter_new(void);
void grease_native_interpreter_free(GreaseNativeInterpreter *interpreter);

GreaseNativeAction grease_native_open_action(
    GreaseNativeHandle directory, const char *path,
    const char *const *flag_names, size_t flag_count, unsigned int mode);
GreaseNativeAction grease_native_close_action(GreaseNativeHandle file);
GreaseNativeAction grease_native_unlink_action(
    GreaseNativeHandle directory, const char *path, bool remove_directory);

GreaseNativeResult grease_native_evaluate(
    GreaseNativeInterpreter *interpreter, const GreaseNativeAction *action);

#endif
