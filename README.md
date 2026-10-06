# grease

A language to smooth the friction between things that come in contact with one another ("shell"), based on Oils/YSH and thickened with Grease-specific changes.

Grease is currently the working Oils/YSH-derived implementation and reference predecessor to `ish`. `ish` is a separate descendant intended to be written in Odriç; it is not a rename of this source tree and it does not make Grease depend on Odriç.

The implementation includes readable boolean operators and [native libc actions](docs/NATIVE-LIBC.md). The [sensor command](commands/sensors/README.md) and accelerometer inspection example are separate consumers; their evidence does not establish compiler-generated or current whole-runtime physical acceptance.

See [`docs/CURRENT-GREASE.md`](docs/CURRENT-GREASE.md) for the language boundary and historical reconciliation of branches, experiments, and receipts.

## Source

The Grease implementation is pinned under [`source/`](source) as a git submodule of [`dilapidated-shed/oils`](https://github.com/dilapidated-shed/oils). The gitlink in each Grease revision is the authoritative source pin; `grease/main` is its development line, not a replacement for that pin. The [receipt workflow](.github/workflows/grease-receipt.yml) verifies and records the exact gitlink before building and running it.

Clone with submodules to obtain the complete source tree:

```sh
git clone --recurse-submodules https://github.com/dilapidated-shed/grease.git
```

Keeping the Oils-derived tree as a pinned submodule avoids copying the full upstream repository while giving Grease one stable, reproducible source location for CI and local builds.
