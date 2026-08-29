# Hackathon Submission — diff2test

## Project name

diff2test

## Track

Track A — Developer Tools & CLI

## Team

`std::zero` — solo

## Bonus claims

- **Single File (+5):** the runtime implementation is one source file, `diff2test.cpp`; tests/docs/fixtures are separate allowed development material.
- **Reproducible Build (+5):** two clean same-runner Release builds were byte-identical; SHA-256 `162a6bbf52034f0c468ab2c7c82853a449590530768e9ed6ddd82f1b7aabc903`.
- **STDLIB Log (+3):** `STDLIB.md` documents 10+ genuine third-party-package substitutions implemented with C++20 standard-library facilities and purpose-built code.
- **Package Killer (+3):** `PACKAGE-KILLER.md` compares `diff2test` with RTS++ / Ekstazi++, an installable C++ regression-test-selection tool whose build requires LLVM and installs its own RTS library/package configuration. `diff2test` replaces that dedicated RTS stack for the narrower supported CMake/CTest metadata workflow without adding third-party runtime code.

The Package Killer claim is intentionally scoped and does **not** claim drop-in equivalence with every RTS++ feature.

## One-line pitch

`diff2test` is a zero-third-party-runtime-dependency C++20 CLI that turns pre-generated CMake/compiler/CTest metadata into an explainable test-impact graph, selects only tests justified by complete evidence, and falls back to the full suite whenever that evidence becomes uncertain.

## Short description

Large C++ projects often run an entire test suite even for small changes. Skipping tests naively is dangerous because incomplete dependency information can hide regressions. `diff2test` takes a conservative alternative: it reads changed paths plus metadata that CMake, GCC/Clang, and CTest already produced outside the process, connects those inputs into a changed-path -> translation-unit -> target -> test graph, and emits a deterministic affected-test list with evidence explanations. A narrow result is allowed only after a completeness/safety audit. Missing, malformed, stale, ambiguous, or unsupported evidence widens to the full known suite; an untrusted CTest catalogue produces `FULL_SUITE_REQUIRED` without inventing names.

## Problem

C++ CI faces an asymmetry:

- running too many tests wastes time;
- running too few tests can silently miss a regression.

Build/test systems already know useful pieces of the dependency graph, but the evidence lives in separate formats and a test-impact tool that shells out to Git/CMake/CTest would violate this hackathon's runtime-dependency constraint.

## Solution

`diff2test` consumes only pre-generated/local inputs:

- changed paths;
- CMake File API codemodel v2 replies;
- explicit GCC/Clang Make-style `.d` dependency files;
- CTest `ctestInfo` JSON v1.

It never invokes their producers.

The core mapping is:

```text
changed path
  -> .d prerequisite evidence
  -> translation unit
  -> owning CMake target
  -> reverse target dependency graph
  -> executable artifact
  -> registered CTest test
```

Every selected test can carry a concrete explanation path.

## Safety model

`diff2test` has three central analysis outcomes:

- `SUBSET_SELECTED` / exit `0`: complete supported evidence justified the selection;
- `FULL_SUITE_SELECTED` / exit `10`: CTest catalogue is known but some other evidence is unsafe, so every known test is emitted;
- `FULL_SUITE_REQUIRED` / exit `11`: the catalogue itself cannot be trusted/enumerated, so no names are invented and the caller is told to run its normal full suite.

The design intentionally prefers false positives over false negatives.

## Zero-dependency implementation

Runtime implementation is one source file:

```text
diff2test.cpp
```

No third-party library, vendored runtime source, package manager, service, network client, or plugin system is used.

Common library capabilities were replaced with C++20 standard-library primitives and purpose-built code, including CLI parsing, strict JSON parsing, UTF-8/Unicode escape validation, path containment, Make-style `.d` parsing, graph representation/traversal, result handling, diagnostics, formatting, and the repository test harness.

See `STDLIB.md` for the full substitution log and `PACKAGE-KILLER.md` for the dedicated RTS comparison.

## CMake clarification

CMake is **permitted external build/input-generation tooling, not a third-party runtime dependency of `diff2test`**.

The organizer explicitly confirmed that compiler/build tooling is allowed and that pre-generated tool output may be parsed when disclosed and handled gracefully when missing. `diff2test` never launches CMake, CTest, Git, a compiler, Python, a shell, or another executable.

## Supported MVP environment

- Linux;
- C++20;
- GCC/Clang Make-style `.d` files;
- CMake File API codemodel major version 2;
- CTest `ctestInfo` JSON major version 1;
- one explicit/unambiguous configuration;
- tested CMake Unix Makefiles-style `CMakeFiles/<target>.dir/...` dependency-target layout;
- direct CTest executable commands with exact artifact matching.

Unsupported or ambiguous cases fall back rather than being guessed through.

## Verified scenarios

Public CI verifies:

1. `include/alpha.hpp` -> exactly `AlphaTest`;
2. `include/features_shared.hpp` -> exactly `AlphaTest` and `BetaTest`;
3. missing required `.d` evidence -> all three fixture tests, exit `10`;
4. missing CTest catalogue -> no names, exit `11`;
5. stdin changed paths == changed-path file behavior;
6. reordered metadata/duplicate changed input -> byte-identical results;
7. 20 repeated real-fixture analyses -> byte-identical stdout/stderr;
8. chain/diamond/cycle target graphs terminate and remain deterministic;
9. detectable stale dependency evidence -> conservative fallback;
10. ASan/UBSan test suite passes;
11. no runtime process-spawn API is found by the CI audit;
12. Linux dynamic linkage contains only system/toolchain runtime libraries;
13. two independent same-runner Release builds are byte-identical.

## Reproducible-build result

In GitHub Actions run `33233072308`, two clean Release builds on the same runner/toolchain produced the identical SHA-256:

```text
162a6bbf52034f0c468ab2c7c82853a449590530768e9ed6ddd82f1b7aabc903
```

This is intentionally a same-environment/toolchain reproducibility claim only.

## Runtime dependency proof

Observed Linux dynamic entries:

```text
linux-vdso.so.1
libstdc++.so.6
libgcc_s.so.1
libc.so.6
libm.so.6
/lib64/ld-linux-x86-64.so.2
```

No CMake library or third-party application library appears. See `DEPENDENCY-PROOF.md` for the complete evidence/reproduction procedure.

## Technical highlights

- strict in-process JSON parser with UTF-8 validation, Unicode escapes/surrogate pairs, duplicate-key rejection, source positions, nesting/input/string limits;
- escape-aware GCC/Clang Make-style dependency parser;
- explicit translation-unit completeness audit;
- exact normalized test-command/artifact mapping—no basename matching;
- lexical root-containment checks without blindly canonicalizing deleted paths;
- project-prerequisite timestamp checks for detectable stale `.d` evidence;
- deterministic target propagation with visited/predecessor tracking;
- concrete per-test evidence chains;
- deliberate nonzero safety exit statuses so CI cannot silently ignore fallback.

## Package Killer rationale

RTS++ / Ekstazi++ is an open-source C++ regression-test-selection tool. Its CMake build requires LLVM, builds an LLVM pass and `ekstazi-lib`, and installs CMake package export/configuration files; its README also identifies separate SHA-512 source as a dependency.

`diff2test` avoids that dedicated RTS package/toolchain for the supported CMake/CTest workflow by consuming metadata that the existing build already produced and implementing the required parser, graph, mapping, and safety logic with the C++20 standard library.

This comparison is documented in `PACKAGE-KILLER.md` with direct source links and an explicit list of capabilities that `diff2test` does **not** claim to replace.

## Limitations

Not claimed:

- arbitrary CMake generators;
- Windows/MSVC dependency formats;
- Ninja `.ninja_deps`;
- wrapper/interpreter CTest commands;
- generated/custom-command dependency chains;
- CMake-language interpretation;
- raw Git patch parsing;
- coverage/history/ML selection;
- absolute proof that metadata matches source contents;
- executing selected tests.

These cases are either outside the MVP or intentionally trigger conservative fallback.

## Repository evidence

Recommended files for judges:

- `README.md` — build, input-generation, usage, safety, limitations;
- `diff2test.cpp` — single runtime implementation;
- `STDLIB.md` — standard-library substitution log;
- `PACKAGE-KILLER.md` — C++ RTS package/tool comparison;
- `DEPENDENCY-PROOF.md` — runtime dependency/reproducibility evidence;
- `SAFETY-CONTRACT.md` — safety rules;
- `fixture/README.md` — controlled real metadata scenarios;
- `WORKLOG.md` — in-window implementation record;
- `.github/workflows/ci.yml` — public verification.

## Demo

The exact under-five-minute recording script and commands are frozen in `DEMO-SCRIPT.md`.

The public video URL should be added to the actual hackathon form (and optionally README) only after the recording/upload is complete and verified.

## License

MIT.

## Preferred shorthand workflow

The primary demonstration uses `git diff --name-only HEAD~1 | diff2test analyze .` after required metadata has been generated externally. This does not make Git a runtime dependency: the caller's shell launches Git and writes newline-delimited paths to stdin; `diff2test` only consumes stdin and existing metadata files. Conventional metadata locations are deterministic defaults, with all explicit legacy flags retained as overrides.
