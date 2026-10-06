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

## Execution attempt — 2026-10-06

At harness revision `6829a9f934f54f8da626bf45b59003a9bc952eed`, the
paired runner was invoked with the explicit roles `greasecpp greased 3`.
It stopped before correctness or timing:

    FAIL greasecpp executable is unavailable: greasecpp

Neither named executable is installed in the sweep environment. Cat Food's
current manifest tracks Oils `grease/main`; its bootstrap resolves Grease
through the pinned Oils/YSH entrypoint, and supplies no completed `greased`
build. The D work is the single translation successor in
[dilapidated-shed/oils PR #5, “Begin whole-hog YSH translation to D”](https://github.com/dilapidated-shed/oils/pull/5).
In particular, the parse-only benchmark requires a real compatible `-n` path.

The runner passes its existing POSIX boundary syntax check. The summarizer was
executed with deliberately unsorted fixture observations: [30, 10, 20] yielded
min/p10/median/p90 = 10/10/20/20, and [40, 20, 30] yielded 20/20/30/30.
These are harness checks, not runtime measurements.

No implementation wins a measured dimension yet. The surviving comparison
dependency is a compatible D executable from that translation line, paired with
the real C++ executable on the same target. This directory remains research
infrastructure; it does not establish a canonical operational workflow.
Repeatable process orchestration belongs in Flexible Pipes, pasteable operator
procedures in Kitchen, and build/deployment identities in Cat Food.
