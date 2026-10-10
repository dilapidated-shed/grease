#!/bin/sh
# LOCAL diagnostic only. This is not a substitute for ai-ci acceptance.
set -eu

if [ -z "${ICK_CC:-}" ]; then
  echo "BLOCKED: set ICK_CC to the absolute path of the owned ICK C compiler" >&2
  exit 2
fi
case "$ICK_CC" in
  /*) ;;
  *) echo "BLOCKED: ICK_CC must be an absolute path" >&2; exit 2 ;;
esac
if [ ! -x "$ICK_CC" ]; then
  echo "BLOCKED: ICK_CC is not executable" >&2
  exit 2
fi

worktree=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

# U+2190 and U+00D7 in the maintained sources must be accepted by ICK itself.
"$ICK_CC" -std=c11 -O2 -Wall -Wextra -Werror -Wpedantic \
  -I"$worktree" \
  "$worktree/grease_actions.c" "$worktree/tests/grease_actions_test.c" \
  -o "$scratch/grease-actions-test"
"$scratch/grease-actions-test"
echo "LOCAL-ONLY: runtime fixture executed; compiler provenance and Grease parity require independent receipts"
