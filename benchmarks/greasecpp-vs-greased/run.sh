#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd -P)
repo_root=$(CDPATH= cd "$script_dir/../.." && pwd -P)
manifest="$script_dir/cases.tsv"

if [ -t 1 ]; then
  cyan='\033[36m'
  yellow='\033[33m'
  green='\033[32m'
  red='\033[31m'
  reset='\033[0m'
else
  cyan=''
  yellow=''
  green=''
  red=''
  reset=''
fi

heading() {
  printf '%b%s%b\n' "$cyan" "$1" "$reset"
}

attention() {
  printf '%b%s%b\n' "$yellow" "$1" "$reset"
}

pass() {
  printf '%bPASS%b %s\n' "$green" "$reset" "$1"
}

fail() {
  printf '%bFAIL%b %s\n' "$red" "$reset" "$1" >&2
  exit 1
}

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
  printf 'usage: sh %s GREASECPP GREASED [REPETITIONS]\n' "$0" >&2
  exit 2
fi

resolve_executable() {
  candidate=$1
  case "$candidate" in
    */*)
      if [ ! -x "$candidate" ]; then
        return 1
      fi
      candidate_dir=$(CDPATH= cd "$(dirname "$candidate")" && pwd -P)
      printf '%s/%s\n' "$candidate_dir" "$(basename "$candidate")"
      ;;
    *)
      command -v "$candidate"
      ;;
  esac
}

if greasecpp=$(resolve_executable "$1"); then
  :
else
  fail "greasecpp executable is unavailable: $1"
fi

if greased=$(resolve_executable "$2"); then
  :
else
  fail "greased executable is unavailable: $2"
fi

repetitions=${3:-21}
case "$repetitions" in
  ''|*[!0-9]*) fail "repetitions must be a positive integer" ;;
esac
if [ "$repetitions" -lt 1 ]; then
  fail "repetitions must be at least 1"
fi

warmups=${WARMUP_REPETITIONS:-3}
case "$warmups" in
  ''|*[!0-9]*) fail "WARMUP_REPETITIONS must be a nonnegative integer" ;;
esac

clock_probe=$(date +%s%N)
case "$clock_probe" in
  *N*|'') fail "date does not provide nanosecond timestamps with +%s%N" ;;
esac

sha256_file() {
  file=$1
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$file" | awk '{print $1}'
    return
  fi
  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$file" | awk '{print $1}'
    return
  fi
  fail "need sha256sum or shasum to identify benchmark executables"
}

run_case() {
  executable=$1
  mode=$2
  case_file=$3
  case "$mode" in
    run) "$executable" "$case_file" ;;
    parse) "$executable" --ast-format none -n "$case_file" ;;
    *) fail "unknown benchmark mode: $mode" ;;
  esac
}

timestamp=$(date -u +%Y%m%dT%H%M%SZ)
results_root=${RESULTS_DIR:-"$script_dir/results/$timestamp"}
mkdir -p "$results_root"

metadata="$results_root/metadata.tsv"
correctness="$results_root/correctness.tsv"
runs="$results_root/runs.tsv"
summary="$results_root/summary.tsv"
resources="$results_root/resources.tsv"

heading "Grease benchmark identities"
printf 'field\tvalue\n' > "$metadata"
printf 'timestamp_utc\t%s\n' "$timestamp" >> "$metadata"
printf 'uname\t%s\n' "$(uname -a)" >> "$metadata"
printf 'repetitions\t%s\n' "$repetitions" >> "$metadata"
printf 'warmup_repetitions\t%s\n' "$warmups" >> "$metadata"

if command -v git >/dev/null 2>&1 && git -C "$repo_root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  if repo_revision=$(git -C "$repo_root" rev-parse HEAD 2>/dev/null); then
    printf 'repository_revision\t%s\n' "$repo_revision" >> "$metadata"
  fi
  if [ -e "$repo_root/source" ] && source_revision=$(git -C "$repo_root/source" rev-parse HEAD 2>/dev/null); then
    printf 'source_revision\t%s\n' "$source_revision" >> "$metadata"
  fi
fi

for implementation in greasecpp greased; do
  if [ "$implementation" = greasecpp ]; then
    executable=$greasecpp
  else
    executable=$greased
  fi
  digest=$(sha256_file "$executable")
  bytes=$(wc -c < "$executable" | tr -d '[:space:]')
  printf '%s_path\t%s\n' "$implementation" "$executable" >> "$metadata"
  printf '%s_sha256\t%s\n' "$implementation" "$digest" >> "$metadata"
  printf '%s_bytes\t%s\n' "$implementation" "$bytes" >> "$metadata"
  printf '%-10s %s  %s bytes\n' "$implementation" "$digest" "$bytes"
done

printf 'case\timplementation\texit_status\tstdout_sha256\tstderr_sha256\n' > "$correctness"

heading "Correctness gate"
while IFS="$(printf '\t')" read -r case_name mode expected_stdout purpose; do
  if [ "$case_name" = case ]; then
    continue
  fi
  case_file="$script_dir/cases/$case_name"
  if [ ! -f "$case_file" ]; then
    fail "missing benchmark case: $case_file"
  fi

  for implementation in greasecpp greased; do
    if [ "$implementation" = greasecpp ]; then
      executable=$greasecpp
    else
      executable=$greased
    fi

    stdout_file="$results_root/$case_name.$implementation.stdout"
    stderr_file="$results_root/$case_name.$implementation.stderr"
    expected_file="$results_root/$case_name.expected"

    if run_case "$executable" "$mode" "$case_file" >"$stdout_file" 2>"$stderr_file"; then
      status=0
    else
      status=$?
    fi

    stdout_hash=$(sha256_file "$stdout_file")
    stderr_hash=$(sha256_file "$stderr_file")
    printf '%s\t%s\t%s\t%s\t%s\n'       "$case_name" "$implementation" "$status" "$stdout_hash" "$stderr_hash" >> "$correctness"

    if [ "$status" -ne 0 ]; then
      fail "$implementation failed $case_name with exit status $status"
    fi
    if [ -s "$stderr_file" ]; then
      # The reference reports this informational marker for --ast-format none.
      # Accept only that exact parse-only marker, never arbitrary diagnostics.
      expected_stderr="$results_root/$case_name.expected-stderr"
      printf 'AST not printed.\n' > "$expected_stderr"
      if [ "$mode" != parse ] || ! cmp -s "$expected_stderr" "$stderr_file"; then
        fail "$implementation wrote unexpected stderr in $case_name"
      fi
    fi

    if [ "$expected_stdout" = '<empty>' ]; then
      if [ -s "$stdout_file" ]; then
        fail "$implementation produced unexpected stdout in $case_name"
      fi
    else
      printf '%s\n' "$expected_stdout" > "$expected_file"
      if cmp -s "$expected_file" "$stdout_file"; then
        :
      else
        fail "$implementation stdout did not match the fixed oracle in $case_name"
      fi
    fi
  done
  pass "$case_name"
done < "$manifest"

heading "Warmup"
while IFS="$(printf '\t')" read -r case_name mode expected_stdout purpose; do
  if [ "$case_name" = case ]; then
    continue
  fi
  case_file="$script_dir/cases/$case_name"
  i=1
  while [ "$i" -le "$warmups" ]; do
    run_case "$greasecpp" "$mode" "$case_file" >/dev/null 2>/dev/null
    run_case "$greased" "$mode" "$case_file" >/dev/null 2>/dev/null
    i=$((i + 1))
  done
done < "$manifest"
pass "warmup complete"

printf 'case\timplementation\titeration\twall_ns\n' > "$runs"

measure_and_record() {
  implementation=$1
  executable=$2
  mode=$3
  case_file=$4
  case_name=$5
  iteration=$6

  start_ns=$(date +%s%N)
  if run_case "$executable" "$mode" "$case_file" >/dev/null 2>/dev/null; then
    :
  else
    fail "$implementation failed during measured run of $case_name"
  fi
  end_ns=$(date +%s%N)
  wall_ns=$((end_ns - start_ns))
  printf '%s\t%s\t%s\t%s\n'     "$case_name" "$implementation" "$iteration" "$wall_ns" >> "$runs"
}

heading "Wall-clock measurements"
while IFS="$(printf '\t')" read -r case_name mode expected_stdout purpose; do
  if [ "$case_name" = case ]; then
    continue
  fi
  attention "$case_name"
  case_file="$script_dir/cases/$case_name"
  i=1
  while [ "$i" -le "$repetitions" ]; do
    if [ $((i % 2)) -eq 1 ]; then
      measure_and_record greasecpp "$greasecpp" "$mode" "$case_file" "$case_name" "$i"
      measure_and_record greased "$greased" "$mode" "$case_file" "$case_name" "$i"
    else
      measure_and_record greased "$greased" "$mode" "$case_file" "$case_name" "$i"
      measure_and_record greasecpp "$greasecpp" "$mode" "$case_file" "$case_name" "$i"
    fi
    i=$((i + 1))
  done
done < "$manifest"

awk -f "$script_dir/summarize.awk" "$runs" > "$summary"
pass "wall-clock measurements complete"

time_bin=${TIME_BIN:-}
if [ -z "$time_bin" ] && [ -x /usr/bin/time ]; then
  time_bin=/usr/bin/time
fi
if [ -z "$time_bin" ] && [ -n "${PREFIX:-}" ] && [ -x "$PREFIX/bin/time" ]; then
  time_bin="$PREFIX/bin/time"
fi
if [ -z "$time_bin" ]; then
  if candidate_time=$(command -v gtime 2>/dev/null); then
    time_bin=$candidate_time
  fi
fi

if [ -n "$time_bin" ]; then
  resource_repetitions=${RESOURCE_REPETITIONS:-3}
  case "$resource_repetitions" in
    ''|*[!0-9]*) fail "RESOURCE_REPETITIONS must be a positive integer" ;;
  esac
  if [ "$resource_repetitions" -lt 1 ]; then
    fail "RESOURCE_REPETITIONS must be at least 1"
  fi

  heading "Resource measurements"
  printf 'case\timplementation\titeration\tuser_sec\tsystem_sec\tmax_rss_kb\tmajor_faults\tminor_faults\n' > "$resources"

  while IFS="$(printf '\t')" read -r case_name mode expected_stdout purpose; do
    if [ "$case_name" = case ]; then
      continue
    fi
    case_file="$script_dir/cases/$case_name"
    for implementation in greasecpp greased; do
      if [ "$implementation" = greasecpp ]; then
        executable=$greasecpp
      else
        executable=$greased
      fi
      i=1
      while [ "$i" -le "$resource_repetitions" ]; do
        usage_file="$results_root/.usage"
        case "$mode" in
          run)
            if "$time_bin" -f '%U\t%S\t%M\t%F\t%R' -o "$usage_file"                 "$executable" "$case_file" >/dev/null 2>/dev/null; then
              :
            else
              fail "resource measurement failed for $implementation $case_name"
            fi
            ;;
          parse)
            if "$time_bin" -f '%U\t%S\t%M\t%F\t%R' -o "$usage_file"                 "$executable" -n "$case_file" >/dev/null 2>/dev/null; then
              :
            else
              fail "resource measurement failed for $implementation $case_name"
            fi
            ;;
        esac
        IFS="$(printf '\t')" read -r user_sec system_sec max_rss_kb major_faults minor_faults < "$usage_file"
        printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n'           "$case_name" "$implementation" "$i" "$user_sec" "$system_sec"           "$max_rss_kb" "$major_faults" "$minor_faults" >> "$resources"
        i=$((i + 1))
      done
    done
  done < "$manifest"
  rm -f "$results_root/.usage"
  pass "resource measurements complete"
else
  attention "GNU time not found; RSS/CPU/page-fault pass is NOT VERIFIED"
fi

heading "Summary"
cat "$summary"
printf '\nresults: %s\n' "$results_root"
