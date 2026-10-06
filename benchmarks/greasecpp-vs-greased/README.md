# greasecpp versus greased benchmarks

This directory compares two real Grease executables against the same programs:

- greasecpp: the C++/MyCPP implementation under test;
- greased: the D implementation under test.

The names describe benchmark roles, not repository aliases. The runner accepts the
two executable paths explicitly and records their SHA-256 identities. It never
falls back to Bash, POSIX sh, Python, another YSH build, or a handwritten
equivalent.

Correctness is a gate. Every case has a fixed expected stdout and requires exit
status 0. Stderr must be empty except for the exact reference informational
marker `AST not printed.` in parse-only mode. Both executables are checked
before timing begins. A semantic mismatch stops the run.

## Cases

| Case | Intended pressure |
| --- | --- |
| 01_empty.ysh | fresh-process startup and runtime initialization |
| 02_parse_large.ysh | parser/frontend cost with --ast-format none -n |
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

The sweep also corrected even-sized sample medians: unsorted observations
[40, 10, 30, 20] previously reported 20 and now report the conventional median
25. Odd-sized fixture results remain unchanged. p10 and p90 select the lower
observed order statistic at `floor((n - 1) * p) + 1`; they do not interpolate.
Repository identity collection now accepts Git worktrees as well as checkouts.

## Paired compatibility execution — 2026-10-06

The suite was then executed against real exact-source CI artifacts on the same
Linux amd64 scratch host, with three requested repetitions:

| Role | Exact source | Executable SHA-256 | Bytes | Build |
| --- | --- | --- | --- | --- |
| greasecpp | Oils f20c8a333d82b20781e1d024c0293dd4acf4ba12 | c2732dca67fa726858a8dd2f2bb5e5f8262aa0f5749dc62180551a618004196a | 17837416 | inherited cxx-asan reference |
| greased | Oils 6db04a02228ad4328f75f79c0ad477c02723767c | 5d7b5bb1e95b718e4de3335c6727c698fc904db1202353e111114a2248678046 | 2654792 | partial YSH D translation, pinned Icky DMD/runtime |

The role name `greased` does not claim this partial translation is complete.
Artifact producers: [Grease reference receipt](https://github.com/dilapidated-shed/grease/actions/runs/37474007912)
and [D compilation, module tests and full smoke sequence](https://github.com/dilapidated-shed/oils/actions/runs/37475808606).
The C++ source implementation is identical to the later alias test-fixture fix.
Leak detection was disabled because LeakSanitizer cannot inspect processes in
this execution environment; no other runtime was substituted.

The first execution exposed a harness defect: `-n` prints the reference AST,
contradicting the empty-output oracle. The repaired parse-only command uses
`--ast-format none -n` and accepts only the exact informational stderr marker,
or empty stderr. Arbitrary diagnostics still fail.

With that repair, both executables pass the empty program. The C++ reference
passes the large parse-only program; D exits 3 with `unexpected 'none' at byte
13`, because it does not implement this CLI/parse-only contract. The runner
stops before warmup or timing. The exact gate hashes are preserved in
[the compatibility receipt](receipts/2026-10-06-compatibility.tsv).
Tested runner SHA-256: `7815e9bbab75ff8fe1e99f7e7ac6e628e928b612a5bc6b75a6d9d1ce7f0fa978`.

Result: these builds are not semantically comparable across the suite yet.
No startup, parser, loop, process, pipeline, CPU or RSS winner is established.
The executable byte counts identify these artifacts; the ASAN C++ build and
partial D build are not equivalent configurations, so those counts do not
establish a language/backend size advantage. The one comparison dependency is
the compatible D runtime, beginning with the nonexecuting parse-only entrypoint.
