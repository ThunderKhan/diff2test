# Minimum Viable Product — diff2test

## MVP statement

Within the 72-hour event, ship one readable C++20 runtime implementation file that safely selects affected CTest tests for a deliberately supported class of CMake projects using changed paths, File API codemodel data, GCC/Clang-style dependency files, and pre-generated CTest JSON—without launching external programs.

## Frozen supported environment

- Linux;
- C++20;
- GCC or Clang producing Make-style `.d` files;
- CMake File API codemodel major version 2;
- one explicitly chosen or unambiguous build configuration;
- CTest `ctestInfo` JSON major version 1;
- CMake Unix Makefiles-style object/dependency layout matching the tested `CMakeFiles/<target>.dir/...` boundary;
- tests whose command executable directly and uniquely matches a CMake executable artifact;
- UTF-8/ASCII project paths without embedded newline or NUL.

Unsupported/ambiguous shapes trigger conservative fallback rather than heuristic selection.

## Required inputs

1. project root;
2. build root;
3. changed paths through a file or stdin;
4. CMake File API reply directory;
5. pre-generated CTest JSON file;
6. explicit `--dep-list` file containing the compiler `.d` files to trust;
7. optional explicit configuration name when more than one codemodel configuration exists.

The kickoff spike deliberately rejected recursive `--dep-root` discovery for the MVP after observing non-compilation `.d` files such as `link.d` in the build tree.

## Required capabilities

### P0-1: CLI foundation — complete

- `analyze`, `--help`, `--version`;
- required-option validation;
- file or stdin changed-path input;
- deterministic names output and readable human output;
- stable safety/usage exit statuses.

### P0-2: JSON parser — complete

- objects, arrays, strings, numbers, booleans, null;
- JSON escapes including `\uXXXX` and surrogate-pair handling;
- UTF-8 validation;
- nesting/input/string-size guards;
- source-position diagnostics;
- no third-party parser.

### P0-3: CMake File API loader — complete

- select a supported reply index;
- follow the codemodel-v2 reference;
- select one configuration;
- load referenced target JSON objects safely;
- collect target id/name/type, sources, artifacts, and dependencies required by the MVP;
- reject root/reference/generated-source cases outside the supported model.

### P0-4: dependency-file loader — complete

- explicit list input;
- Make rule separator;
- backslash-newline continuation;
- escaped whitespace/backslash/hash/colon handling needed for tested GCC/Clang output;
- multiple prerequisites;
- deterministic deduplication;
- malformed/incomplete result;
- `(target, compiled source)` coverage audit;
- detectable timestamp-staleness fallback for project prerequisites.

### P0-5: CTest loader and mapping — complete

- validate `kind` and major version;
- collect every registered test name and command;
- exact normalized executable-artifact mapping;
- zero/multiple matches and wrapper-style commands are unsafe.

### P0-6: graph and selection — complete

- changed source/header -> translation units;
- translation units -> owning targets;
- reverse target dependency propagation;
- affected executable targets -> registered tests;
- at least one deterministic evidence/explanation path per selected test;
- chain/diamond/cycle traversal tested.

### P0-7: safety fallback — complete

- audit required evidence before committing a narrow result;
- full known test list when the catalogue exists but another evidence source is unsafe;
- `FULL_SUITE_REQUIRED` when the catalogue cannot be trusted/enumerated;
- no invented names;
- concrete fallback reason.

### P0-8: verification and submission — engineering side complete

- automated parser/loader/graph/safety tests;
- realistic integration fixture created after kickoff;
- clean one-command build;
- public CI clean-room verification;
- dependency proof;
- `README.md` and `STDLIB.md`;
- reproducible same-runner Release build proof;
- exact five-minute demo script.

The remaining demo **recording/upload** itself is a human submission action and is intentionally not marked as automated repository work.

## MVP data model

Conceptual nodes:

- normalized path;
- translation unit;
- build target;
- target artifact;
- registered test.

Conceptual relationships:

- dependency-file prerequisite -> translation unit;
- translation unit -> owning target;
- target -> dependent target for the modeled CMake relation;
- executable target artifact -> registered test command.

Explanation output retains the evidence origin needed to show a concrete chain from a changed path to a selected test.

## MVP safety policy

Selective output is permitted only when the supported evidence audit succeeds. The implementation requires, among other checks:

- syntactically valid, non-empty changed input;
- every changed path known to the current supported evidence model;
- supported metadata major versions;
- unambiguous configuration selection;
- complete trusted dependency evidence for every in-scope compiled translation unit;
- no detectable stale project prerequisite relative to its `.d` file;
- every registered test maps exactly once to a supported executable artifact;
- all referenced target edges are known;
- no unsupported generated/custom relationship is relied upon;
- normalized roots/metadata references remain within the declared supported boundaries.

Otherwise the program emits the full known suite or `FULL_SUITE_REQUIRED` according to whether the CTest catalogue itself can be trusted.

## Controlled demo project — complete

The fixture contains:

- one shared `core` library;
- independent `alpha` and `beta` feature libraries;
- three direct executable CTest tests (`AlphaTest`, `BetaTest`, `CoreTest`);
- `alpha.hpp` for a narrow alpha-only change;
- `features_shared.hpp` used by alpha and beta but not core;
- `common.hpp` participating in the wider shared graph;
- generated File API/compiler/CTest metadata;
- a reproducible way to remove dependency evidence for fallback.

Public CI verifies:

1. `alpha.hpp` -> exactly `AlphaTest`;
2. `features_shared.hpp` -> exactly `AlphaTest` + `BetaTest`;
3. deterministic explanation/output;
4. missing dependency evidence -> full known suite with exit `10`;
5. missing CTest catalogue -> no names and exit `11`.

## Quality floor — verified

- no uncaught exception crosses `main`;
- no silent parser recovery after malformed structural input;
- no raw unordered-container output dependency;
- fallback has explicit non-success safety status;
- unsupported cases are not guessed through;
- no runtime external command invocation exists;
- P0 behavior is covered by unit/component/integration/CI checks;
- ASan/UBSan test runs pass in dev-only CI;
- real output is byte-stable across reordered evidence and repeated runs.

## Deliberate scope cuts

The final MVP intentionally does **not** implement:

- JSON output mode;
- ANSI color;
- `compile_commands.json`;
- recursive `.d` discovery;
- configurable ignore patterns;
- Ninja/MSVC dependency formats;
- wrapper/interpreter test commands;
- generated/custom-command dependency chains;
- raw Git patch parsing;
- coverage/history/ML selection;
- executing selected tests.

These cuts keep the evidence model auditable and conservative.

## Reproducibility result

Two independent same-runner/toolchain clean Release builds in public CI were byte-identical and shared SHA-256:

```text
162a6bbf52034f0c468ab2c7c82853a449590530768e9ed6ddd82f1b7aabc903
```

This is a same-environment/toolchain result, not a cross-platform reproducibility claim.

## Definition of MVP complete

### Engineering/repository completion

The engineering MVP is complete when:

- P0 tests pass from a clean checkout/build;
- the supported demo produces verified narrow and shared-header selections;
- designed evidence failures produce the documented safe outcomes;
- output/status behavior matches the frozen CLI and safety contracts;
- docs explain external metadata generation and runtime/tooling boundaries;
- no third-party runtime code or vendored source exists;
- dependency/reproducibility proof is recorded.

These conditions are currently satisfied and summarized in `FINAL-AUDIT.md`.

### Human submission completion

The overall hackathon submission is complete only after the entrant additionally:

- rehearses `DEMO-SCRIPT.md` twice;
- records an under-five-minute demo;
- verifies/uploads the video;
- adds the final public video URL to the submission;
- submits the final hackathon form before the deadline.

Those actions cannot be replaced by CI or repository automation.
