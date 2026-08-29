# Five-Minute Demo Contract

## Objective

In under five minutes, prove usefulness, zero-dependency craft, explainability, deterministic selection, and conservative failure—without forcing the judge through a long setup tutorial.

The exact command-by-command recording script is frozen in [`DEMO-SCRIPT.md`](DEMO-SCRIPT.md). This contract defines what the final video must prove.

## Narrative

> Running every C++ test is safe but slow. Guessing which tests to skip is fast but dangerous. diff2test reads evidence the build already produced, selects only what it can justify, and asks for the full suite the moment that evidence breaks.

## Required environment

- clean public repository checkout;
- supported Linux/GNU-or-Clang toolchain;
- readable terminal;
- no hidden aliases/functions that alter commands;
- CMake/CTest/compiler metadata generated externally using the documented commands;
- reversible missing-evidence mutation.

## Timeline

### 0:00–0:20 — Hook

State the problem and safety asymmetry.

### 0:20–0:45 — Artifact/rule proof

Show:

- `diff2test.cpp` as the single runtime implementation source;
- separate tests/fixture/docs;
- no vendored third-party runtime code;
- CMake classified as external build/input-generation tooling;
- the runtime never launches Git/CMake/CTest/compiler/shell.

### 0:45–1:15 — Clean build

Use the exact Release build in `DEMO-SCRIPT.md` and show the produced executable.

### 1:15–1:40 — Dependency proof

Show `ldd build/diff2test` and explain the observed system/toolchain runtime entries. Show the process-spawn source audit or `DEPENDENCY-PROOF.md`.

### 1:40–2:45 — Narrow selection

Generate/use complete fixture metadata, change `include/alpha.hpp`, and run human mode with `--explain`.

Required visible facts:

- `SUBSET_SELECTED`;
- `AlphaTest` selected;
- concrete changed-path -> dependency-file -> translation-unit -> target -> test evidence chain.

### 2:45–3:15 — Shared-header selection

Change `include/features_shared.hpp`.

Required names:

```text
AlphaTest
BetaTest
```

This real scenario is already enforced by public CI.

### 3:15–4:00 — Missing-evidence fallback

Use the documented incomplete dependency-list copy.

Required facts:

- all three known fixture tests emitted;
- exit `10` / `FULL_SUITE_SELECTED`;
- concrete reason on stderr.

### 4:00–4:25 — Missing catalogue

Point `--ctest-info` at a missing file.

Required facts:

- no invented names;
- exit `11` / `FULL_SUITE_REQUIRED`.

### 4:25–4:50 — Close

Show `STDLIB.md` / `DEPENDENCY-PROOF.md` and close with:

> The optimization is conditional; the safety behavior is not. Complete evidence gives a smaller suite. Uncertainty gives everything—or explicitly asks the caller to run everything when the catalogue itself cannot be trusted.

## Frozen command sources

The final exact commands now exist in:

1. `README.md` — build, metadata generation, usage, safety behavior;
2. `fixture/README.md` — narrow/shared/fallback fixture scenarios;
3. `DEPENDENCY-PROOF.md` — linkage/subprocess/reproducibility proof;
4. `DEMO-SCRIPT.md` — recording order and exact terminal commands.

There is no remaining “fill commands later” placeholder in the demo plan.

## Required visible evidence

- public repository created/implemented inside the event window;
- single runtime implementation file;
- clean build;
- executable runs;
- no runtime subprocess behavior;
- narrow real selection;
- shared-header real selection;
- evidence explanation;
- full-known fallback;
- catalogue-failure behavior;
- runtime dependency proof;
- `README.md` and `STDLIB.md`.

## Fixture constraints — satisfied

- three direct CTest tests: `AlphaTest`, `BetaTest`, `CoreTest`;
- alpha-only header for meaningful one-test selection;
- `features_shared.hpp` for two-test selection;
- removable dependency evidence;
- understandable names/output;
- public CI verifies the substantive scenarios.

## Recording rules

- target 4:30–4:50;
- 1080p if practical, terminal font 18+;
- hide notifications/unrelated personal data;
- rehearse twice from a clean checkout;
- keep the original local recording until submission is accepted;
- avoid speed-ups that obscure evidence;
- explain external metadata generation honestly;
- verify the uploaded video is publicly accessible before submission.

## Failure contingencies

| Failure | Response |
|---|---|
| clean build fails | stop recording and fix; do not substitute an old artifact |
| fixture selection differs from CI | stop recording and diagnose; do not fake output |
| metadata generation is too verbose | generate before the take and show the documented commands/files |
| `ldd` is confusing | show `DEPENDENCY-PROOF.md` alongside the actual output |
| recording is unreadable/corrupt | record another take from the same frozen script |

## Automated readiness

The repository already automates the engineering side of the demo through public CI: clean checkout/build, real metadata generation, narrow/shared selection, deterministic output, missing-evidence fallback, catalogue failure, dependency audit, and same-toolchain reproducible Release builds.

## Human acceptance checklist

These boxes can only be completed by the entrant while recording/submitting:

- [ ] rehearsed from a clean checkout — first pass
- [ ] rehearsed from a clean checkout — second pass
- [ ] final video under five minutes
- [ ] hook within first 20 seconds
- [ ] clean build visible
- [ ] single-file implementation visible
- [ ] zero-runtime-dependency evidence visible
- [ ] narrow selection + explanation visible
- [ ] shared-header selection visible
- [ ] full-known fallback visible
- [ ] catalogue failure shown or clearly described if time-constrained
- [ ] no claim exceeds documented support
- [ ] `STDLIB.md` / dependency proof called out
- [ ] uploaded URL verified public
