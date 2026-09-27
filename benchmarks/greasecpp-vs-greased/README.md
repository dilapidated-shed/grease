# greasecpp versus greased benchmarks

This directory compares two real Grease executables against the same programs:

- greasecpp: the C++/MyCPP implementation under test;
- greased: the D implementation under test.

The names describe benchmark roles, not repository aliases. The runner accepts the
two executable paths explicitly and records their SHA-256 identities. It never
falls back to Bash, POSIX sh, Python, another YSH build, or a handwritten
equivalent.

Correctness is a gate. Every case has a fixed expected stdout, requires empty
stderr and exit status 0, and is checked for both executables before timing
begins. A semantic mismatch stops the run.

## Cases

| Case | Intended pressure |
| --- | --- |
| 01_empty.ysh | fresh-process startup and runtime initialization |
| 02_parse_large.ysh | parser/frontend cost with execution disabled by -n |
| 03_loop.ysh | typed integer range/loop/update overhead |
| 04_function_call.ysh | repeated Grease proc dispatch |
| 05_string.ysh | immutable string construction and length |
| 06_list.ysh | typed List creation, iteration and len() |
| 07_external_command.ysh | repeated external process launch |
| 08_pipeline.ysh | repeated pipe setup plus external commands |

The empty case is a fresh-process measurement, not a claim about cold kernel
page cache or cold storage. If cold-cache behavior matters, control that
separately and record the condition.

## Run

From any working directory:

    sh /path/to/grease/benchmarks/greasecpp-vs-greased/run.sh \
      /path/to/greasecpp /path/to/greased

The optional third argument is the number of measured repetitions; the default
is 21. WARMUP_REPETITIONS defaults to 3.

Results are written below results/ and ignored by git. The runner records:

- executable path, SHA-256 and byte size;
- repository and source-submodule revisions when available;
- exact correctness status and stdout/stderr hashes;
- one wall-clock nanosecond observation per case, implementation and repetition;
- min, p10, median and p90 wall time.

Execution order alternates on each repetition instead of always running one
implementation first.

If GNU time is available at /usr/bin/time, or at $PREFIX/bin/time on Termux, a
separate resource pass records user CPU seconds, system CPU seconds, maximum RSS,
major page faults and minor page faults. Set TIME_BIN explicitly to use another
GNU-time executable. Resource measurements are separate from wall measurements
so the timing wrapper does not contaminate the startup samples.

## Comparison rules

Use the same machine, filesystem, workload files and surrounding load for a
paired run. Record build configuration separately; optimized and debug builds
are not interchangeable. Do not collapse the suite into one overall score:
startup, parser work, interpreter work, process creation, pipelines, RSS and
binary size answer different questions.

For the MIRO A1 in particular, fresh-process latency, maximum RSS and executable
size are first-class results because Grease may be invoked repeatedly from other
programs. Long-lived execution is represented by the loop, proc, string and list
cases; later benchmarks can add larger real Grease workloads without changing
the startup case.

The suite is benchmark infrastructure, not acceptance evidence for a D
translation by itself. A run is evidence only for the exact executable hashes
and source revisions written into that run's metadata.
