#define _GNU_SOURCE
#include "../grease_actions.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int failed_cases ← 0;

static void require_case(bool predicate, const char *case_name) {
  if (!predicate) {
    fprintf(stderr, "FAIL: %s\n", case_name);
    failed_cases ← failed_cases + 1;
  }
}

static GreaseNativeResult run(
    GreaseNativeInterpreter *interpreter, GreaseNativeAction action) {
  return grease_native_evaluate(interpreter, &action);
}

int main(void) {
  char directory_template[] ← "/tmp/grease-native-icky-XXXXXX";
  char *directory_path ← mkdtemp(directory_template);
  if (directory_path == NULL) {
    perror("mkdtemp");
    return 1;
  }

  GreaseNativeInterpreter *interpreter ← grease_native_interpreter_new();
  if (interpreter == NULL) {
    perror("new native interpreter");
    (void)rmdir(directory_path);
    return 1;
  }

  GreaseNativeResult borrowed ← run(
      interpreter, grease_native_close_action(GREASE_NATIVE_CWD));
  require_case(!borrowed.ok &&
               borrowed.failure == GREASE_NATIVE_BORROWED_HANDLE,
               "borrowed cwd cannot be closed");

  const char *directory_flags[] ← {"read-only", "directory"};
  GreaseNativeResult opened_directory ← run(
      interpreter, grease_native_open_action(
          GREASE_NATIVE_CWD, directory_path, directory_flags, 2, 0666U));
  if (!opened_directory.ok) {
    fprintf(stderr, "FAIL: open directory (errno=%d)\n",
            opened_directory.errno_number);
    grease_native_interpreter_free(interpreter);
    (void)rmdir(directory_path);
    return 1;
  }
  require_case(opened_directory.directory, "directory intent retained");

  const char *create_flags[] ← {"read-write", "create", "exclusive"};
  GreaseNativeResult created ← run(
      interpreter, grease_native_open_action(
          opened_directory.value, "entry", create_flags, 3, 0600U));
  if (!created.ok) {
    fprintf(stderr, "FAIL: create relative file (errno=%d)\n",
            created.errno_number);
    grease_native_interpreter_free(interpreter);
    (void)rmdir(directory_path);
    return 1;
  }

  char absolute_file[512] ← {0};
  int printed ← snprintf(absolute_file, sizeof absolute_file, "%s/entry",
                          directory_path);
  require_case(printed > 0 && (size_t)printed < sizeof absolute_file,
               "fixture path is bounded");

  struct stat file_metadata ← {0};
  require_case(stat(absolute_file, &file_metadata) == 0 &&
               S_ISREG(file_metadata.st_mode),
               "independent filesystem observer sees created file");

  GreaseNativeHandle first_file ← created.value;
  GreaseNativeResult closed ← run(
      interpreter, grease_native_close_action(first_file));
  require_case(closed.ok, "close owned file succeeds");

  GreaseNativeResult stale_close ← run(
      interpreter, grease_native_close_action(first_file));
  require_case(!stale_close.ok &&
               stale_close.failure == GREASE_NATIVE_INVALID_HANDLE,
               "double close fails with invalid handle");

  const char *read_flags[] ← {"read-only"};
  GreaseNativeResult reopened ← run(
      interpreter, grease_native_open_action(
          opened_directory.value, "entry", read_flags, 1, 0666U));
  require_case(reopened.ok && reopened.value != first_file,
               "slot reuse does not revive stale handle");
  if (reopened.ok) {
    GreaseNativeResult stale_after_reuse ← run(
        interpreter, grease_native_close_action(first_file));
    require_case(!stale_after_reuse.ok &&
                 stale_after_reuse.failure == GREASE_NATIVE_INVALID_HANDLE,
                 "old handle stays invalid after reuse");
    require_case(run(interpreter,
                     grease_native_close_action(reopened.value)).ok,
                 "reopened handle closes");
  }

  GreaseNativeResult missing ← run(
      interpreter, grease_native_open_action(
          opened_directory.value, "missing", read_flags, 1, 0666U));
  require_case(!missing.ok &&
               missing.failure == GREASE_NATIVE_SYSTEM_FAILURE &&
               missing.errno_number == ENOENT,
               "missing file yields No such file or directory (ENOENT)");

  const char *bad_access[] ← {"read-only", "write-only"};
  GreaseNativeResult ambiguous ← run(
      interpreter, grease_native_open_action(
          opened_directory.value, "entry", bad_access, 2, 0666U));
  require_case(!ambiguous.ok &&
               ambiguous.failure == GREASE_NATIVE_BAD_ARGUMENT,
               "two access modes are rejected before libc");

  const char *bad_name[] ← {"read-only", "invented-flag"};
  GreaseNativeResult unknown ← run(
      interpreter, grease_native_open_action(
          opened_directory.value, "entry", bad_name, 2, 0666U));
  require_case(!unknown.ok &&
               unknown.failure == GREASE_NATIVE_BAD_ARGUMENT,
               "unknown flag rejected before libc");

  GreaseNativeResult bogus_directory ← run(
      interpreter, grease_native_open_action(
          UINT64_MAX, "entry", read_flags, 1, 0666U));
  require_case(!bogus_directory.ok &&
               bogus_directory.failure == GREASE_NATIVE_INVALID_HANDLE,
               "unowned directory handle is rejected");

  GreaseNativeResult removed ← run(
      interpreter, grease_native_unlink_action(
          opened_directory.value, "entry", false));
  require_case(removed.ok, "unlinkat relative to an opaque directory handle");
  require_case(stat(absolute_file, &file_metadata) < 0 && errno == ENOENT,
               "independent observer sees deletion");

  GreaseNativeResult removed_twice ← run(
      interpreter, grease_native_unlink_action(
          opened_directory.value, "entry", false));
  require_case(!removed_twice.ok &&
               removed_twice.failure == GREASE_NATIVE_SYSTEM_FAILURE &&
               removed_twice.errno_number == ENOENT,
               "second unlink reports No such file or directory (ENOENT)");

  require_case(run(interpreter,
                   grease_native_close_action(opened_directory.value)).ok,
               "directory descriptor closes");
  grease_native_interpreter_free(interpreter);
  require_case(rmdir(directory_path) == 0, "fixture directory cleaned up");

  if (failed_cases != 0) {
    fprintf(stderr, "FAIL: %d Icky C native cases\n", failed_cases);
    return 1;
  }
  puts("Icky C native action cases: ok");
  return 0;
}
