# Native Grease: functorial Icky C, then Icky Lua

Status: candidate first-pass implementation, not a replacement shell and not
an accepted Grease executable. No C++ code, Python launcher, stock C compiler
fallback, or stock Lua interpreter is added here.

## References and independent branches

The authoritative working Grease is the parent repository's source/ gitlink
(Oils/YSH); the reference native action implementation lives at
builtin/func_native.py and cpp/libc.cc in that pinned Oils tree. The source pin
inspected when this candidate began was
5651cf97a1b5042f24f14112a7ade9a1518eb0bc. Every parity receipt must
re-read and record the actual gitlink. Do not treat this directory as
the implementation or as proof of parity with that pin.

The D translation, this functorial Icky C-to-Lua translation, and the separate
Odriç ish successor are independent lines. Do not borrow an acceptance receipt
or silently replace one with another.

ICK compiler source reviewed: dilapidated-shed/ick at
c61e448251744a2f40ad743ebef1a027bdcd2f9d. Icky C accepts the exact
left arrow U+2190 as C assignment/initializer and multiplication sign U+00D7
as C multiplication. These forms appear directly in the owned C sources.
The compiler's build/provenance at this candidate revision is not yet accepted.

Second-pass Lua is fixed to the modified isomorphisms/lua at
87306483cec50f8c750a22dda1d0742246fad756, not ordinary Lua.
That repository has its own symbolic assignment/equality/arithmetic tests.
C execution acceptance is a mandatory predecessor of the Lua execution gate.
The prepared binding and policy remain candidates until those gates pass.

The separate dilapidated-shed/icky Idris2 parser preserves glyphs and source
order, but does NOT parse YSH or assign Grease meaning. This C action interpreter
does not claim parser integration.

## Explicit translation shape

    Grease native action (data)
       -> named Icky C evaluation function
       -> libc or Bionic action
       -> explicit result and owned opaque resource
       -> Icky Lua policy / sequencing candidate

Each action is constructed as a value without effects. Evaluation selects a
named action and returns an explicit success or failure. The C interpreter
owns descriptors, syscall argument validation, error capture, and resource
lifetime. Later Lua may decide which action to request or how to compose
results; it must not recreate the native implementation or forge receipts.

This first slice implements openat, close, and unlinkat. Stable named flags
are translated to host flags. The borrowed cwd handle cannot be closed.
The generation-bearing C token prevents stale descriptors from acquiring new
resources after slot reuse. The Lua binding holds tokens in protected userdata
and retains the owning interpreter. Actions also retain their interpreter and
copy references to immutable path/flag strings, so later mutation of the input
flag table cannot change an already constructed action. Neither tokens nor raw
descriptors are exposed to scripts. Cross-interpreter handles/actions and
zero-containing text are rejected before the native interpreter is invoked.

Lua's composition.sequence validates a dense action list, evaluates each
action through the named C interpreter, and returns the first failure without
running subsequent actions. The demonstration driver loads only Lua's base
library; it does not expose debug registry mutation or native module loading.
This is a controlled migration fixture, not a security sandbox or a shell.
Closing handles remains an explicit C action. Collection of an interpreter
closes any remaining descriptors; live handles/actions retain that interpreter.

Error outcomes are separated into invalid arguments, invalid handles, borrowed
handles, and libc failures with captured errno. No raw fd is returned. Mapping,
mprotect, msync, readMapping, writeMapping, fallocate, linkat, symlinkat,
shell parsing, word/quote expansion, variables, alias/unalias, process launch,
pipelines, and job control are NOT implemented in this slice.

The C core is synchronous and interpreter-local, not a concurrency-safe shared
registry. Porting the higher-level YSH result/object representation is still
required; the C result fields are not an assertion of public YSH object parity.

## Acceptance boundary

The C fixture tests a real temporary directory, relative open/close/unlink,
the external filesystem state, two-access-mode rejection, unknown flags,
No such file or directory (ENOENT), borrowed cwd, double close, and stale
handle rejection after slot reuse.

Run tests/run-ick.grease through the pinned current Grease executable with an
absolute qualified ICK driver, a new output directory, and its declared runtime
flags. The native-icky workflow uses the shared ICK host producer pinned to
c61e448251744a2f40ad743ebef1a027bdcd2f9d and the shared build-toolchain contract.
It runs the unchanged-source positive fixture and separate stale-generation,
multiple-access-mode, and masked-unlink-error mutations. The fixture also grows
the descriptor registry beyond its initial capacity, checks exclusive create,
missing arguments, permission ranges, and symbolic-link refusal.

After that exact-head C gate, tests/run-lua.grease reuses the unchanged shared
modified-Lua qualifier, links its already checked runtime object, verifies raw
embedded policy bytes, and compares one filesystem transcript between the C
pass, the Lua-composed pass, and the pinned working Grease. It tests effect-free
construction, stale handles, opaque userdata, foreign ownership, native text,
first-failure sequencing, retained context lifetime, and descriptor cleanup.
Separate bridge mutants remove handle/action ownership and native disposal;
a policy mutant continues after failure. A real same-version ordinary-Lua
runtime is linked into the same consumer and must reject the raw policy.

This comparison is limited to valid open/close/unlink actions and native errno
results. It does not claim Grease parser, exception, word expansion, or public
object compatibility. Grease's invalid-argument exceptions and this candidate's
explicit failure results remain distinct.

The earlier FastChat host compiler artifact at 37589849631 / 11468256648 accepts
the assignment arrow but rejects multiplication U+00D7. It is therefore not the
producer for this candidate. No glyph lowering or stock-C fallback is allowed.

The runner has no stock compiler fallback and its glyph syntax must be compiled
by ICK. A local success is diagnostic only: it does not certify
the executable was built from the listed ICK source, prove the Grease language
surface, the pinned Oils/YSH behavior, a Lua adapter, Android, or a physical
device. Independent ai-ci toolchain provenance, exact-head compilation,
positive and known-bad executable fixtures, and comparison against pinned
Grease are recorded by the candidate workflow. A pending run remains pending.
Independent Flexible Pipes job admission is still separate. This candidate is
not merge-ready.
