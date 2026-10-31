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

The first hosted continuation at 38ddc14b completes all inherited source
generation tasks, including C++ smoke, tar generation and native build timing.
Its outer bootstrap then fails because host-shim was invoked outside its source
working directory. Running that retained tool in its required directory repairs
the wrapper contract; no generated source or success allowance changes. The
source tar and exact commit survive in run 41252620342, artifact 13061484400,
ZIP SHA-256 694d664b7f7888e8cd30c49a8c966c7f979069835b99b1cee162c8dfbb3a604e.
Android runtime builds/execution remain pending after that failed run.

The next head 28a84bb2 passes source generation and bootstrap, but Android SDK
setup refuses the runner's JDK 11: the pinned command-line tools require JDK 17.
The continuation selects JDK 17 through the same pinned setup action used by the
existing ARM recipe. This is a host SDK prerequisite, not application Java code.
Run 41253417590 therefore provides no target runtime result. Extraction also uses
`--no-same-owner`, since source archive ownership is not an input to a build and
the local managed filesystem cannot adopt its container UID.
