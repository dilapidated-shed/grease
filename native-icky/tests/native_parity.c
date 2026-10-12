#include "../grease_actions.h"
#include <stdio.h>

static GreaseNativeResult evaluate(
    GreaseNativeInterpreter *interpreter, GreaseNativeAction action) {
  return grease_native_evaluate(interpreter, &action);
}

static void status(const char *name, GreaseNativeResult result) {
  printf("%s\t%s\n", name, result.ok ? "true" : "false");
}

int main(int count, char **arguments) {
  if (count != 2) return 2;
  GreaseNativeInterpreter *interpreter ← grease_native_interpreter_new();
  if (interpreter == NULL) return 3;
  const char *directory_flags[] ← {"read-only", "directory"};
  GreaseNativeResult directory ← evaluate(interpreter, grease_native_open_action(
      GREASE_NATIVE_CWD, arguments[1], directory_flags, 2, 0600U));
  if (!directory.ok) return 4;
  const char *create_flags[] ← {"read-write", "create", "exclusive"};
  GreaseNativeResult created ← evaluate(interpreter, grease_native_open_action(
      directory.value, "entry", create_flags, 3, 0600U));
  if (!created.ok) return 5;
  status("created", created);
  GreaseNativeResult duplicate ← evaluate(interpreter, grease_native_open_action(
      directory.value, "entry", create_flags, 3, 0600U));
  printf("exclusive_error\t%d\n", duplicate.errno_number);
  status("closed", evaluate(interpreter, grease_native_close_action(created.value)));
  GreaseNativeResult reopened ← evaluate(interpreter, grease_native_open_action(
      directory.value, "entry", NULL, 0, 0600U));
  if (!reopened.ok) return 6;
  status("reopened", reopened);
  status("reclosed", evaluate(interpreter, grease_native_close_action(reopened.value)));
  GreaseNativeResult missing ← evaluate(interpreter, grease_native_open_action(
      directory.value, "missing", NULL, 0, 0600U));
  printf("missing_error\t%d\n", missing.errno_number);
  status("removed", evaluate(interpreter, grease_native_unlink_action(
      directory.value, "entry", false)));
  GreaseNativeResult absent ← evaluate(interpreter, grease_native_unlink_action(
      directory.value, "entry", false));
  printf("second_unlink_error\t%d\n", absent.errno_number);
  status("directory_closed", evaluate(interpreter, grease_native_close_action(directory.value)));
  grease_native_interpreter_free(interpreter);
  return 0;
}
