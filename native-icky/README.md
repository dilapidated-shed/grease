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
Lua implementation begins only after this C feature set has independent
ICK compilation and behavioral acceptance.

The separate dilapidated-shed/icky Idris2 parser preserves glyphs and source
order, but does NOT parse YSH or assign Grease meaning. This C action interpreter
does not claim parser integration.

## Explicit translation shape

    Grease native action (data)
       -> named Icky C evaluation function
       -> libc or Bionic action
       -> explicit result and owned opaque resource
       -> future Icky Lua policy / sequencing

Each action is constructed as a value without effects. Evaluation selects a
named action and returns an explicit success or failure. The C interpreter
owns descriptors, syscall argument validation, error capture, and resource
lifetime. Later Lua may decide which action to request or how to compose
results; it must not recreate the native implementation or forge receipts.

This first slice implements openat, close, and unlinkat. Stable named flags
are translated to host flags. The borrowed cwd handle cannot be closed.
The generation-bearing C token prevents stale descriptors from acquiring new
resources after slot reuse. A future Lua binding must hold handles in userdata
rather than exposing these C tokens to scripts, and attach their interpreter
identity so cross-interpreter tokens cannot be confused.

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

Run tests/run-ick.sh only with ICK_CC set to an absolute path to a qualified
ICK C compiler. The runner has no stock compiler fallback and its arrow syntax
must be compiled by ICK. A local success is diagnostic only: it does not certify
the executable was built from the listed ICK source, prove the Grease language
surface, the pinned Oils/YSH behavior, a Lua adapter, Android, or a physical
device. Independent ai-ci toolchain provenance, exact-head compilation,
positive and known-bad executable fixtures, and comparison against pinned
Grease remain pending. This candidate is not merge-ready.
