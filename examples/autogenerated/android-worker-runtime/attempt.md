# Android worker runtime for IB

IB must launch its actual Grease supervisor in the authorized Shizuku process.
The maintained phone/tablet recipes already generate the exact pinned Oils/YSH
C++ source and cross-build ARM runtimes. This continuation must reuse that
source owner and add a generic x86_64 qualification lane, rather than executing
a different shell or claiming a host binary is an Android program.

The small configure probe is Grease orchestration of the retained configure
command's compile-only fast mode. Its typed inputs are an absolute generated-source directory
and an absolute Android compiler. Target probes compile but never execute on the
host. Exact source, Android toolchain, native ELF and actual emulator execution
remain separate evidence. ABI labels are generic builds, not tablet observations.

No new language semantics, successor interpreter, permanent release or physical
acceptance is implied by this helper. Source generation and runtime qualification
must be bound to the current repository gitlink before IB can package it.

The first attempt to source configure directly in YSH correctly refused its
POSIX `readonly` builtin. No runtime was built from that attempt. Calling the
maintained configure program through its own interface preserves its language
boundary; it is not counted as Grease execution or target acceptance.
