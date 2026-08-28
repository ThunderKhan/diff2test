# README.md Submission Outline

Use this outline during the hackathon. Replace every placeholder from the actual implementation; do not publish aspirational claims.

## Header

- diff2test
- one-line pitch
- Track A and team `std::zero`
- concise status badges only if they are real

## Problem

- full C++ suites are slow;
- naive selection risks false negatives;
- existing build metadata is fragmented;
- safety principle: uncertainty means full suite.

## What it does

- consumes changed paths;
- parses pre-generated File API, `.d`, CTest JSON;
- maps impact;
- emits selected names/explanations;
- falls back safely;
- never invokes external tools or runs tests.

## 30-second example

Include the exact tested command and representative output from the real fixture.

## Supported environment

- OS/toolchain/generator;
- supported format major versions;
- configuration behavior;
- direct test command mapping requirement.

## How it works

Brief pipeline:

```text
changed path → translation unit → target → artifact → CTest test
```

Explain `.d` transitive header role and safety ledger.

## Build

- prerequisites that are build tools, not runtime dependencies;
- state explicitly that CMake is used only as permitted build tooling and is not invoked or required by the `diff2test` runtime artifact;
- distinguish compiling `diff2test` with CMake from consuming CMake-generated metadata during analysis;
- one exact clean build command;
- produced artifact path;
- optional test command.

## Build-tool vs runtime-dependency boundary

Explain this distinction prominently because judges may reasonably ask whether CMake itself is a dependency:

- the organizer explicitly confirmed that the compiler and build tool are not counted as runtime dependencies;
- CMake may be used to build `diff2test`;
- CMake/CTest may also generate metadata before `diff2test` runs;
- `diff2test` never executes CMake, CTest, Git, a compiler, shell, or other installed program at runtime;
- the shipped executable contains no CMake library and does not require a CMake process to analyze already-produced files;
- if CMake/CTest metadata is absent or unusable, `diff2test` degrades conservatively to `FULL_SUITE_SELECTED` or `FULL_SUITE_REQUIRED` rather than trying to invoke the missing tool.

Do not use the vague claim “CMake is not a dependency” without qualification. The precise claim is: **CMake is permitted external build/input-generation tooling, not a third-party runtime dependency of the shipped artifact.**

## Generate inputs externally

Document commands for:

- File API query before CMake configure;
- configure/build that produces `.d`;
- `ctest --show-only=json-v1` export;
- changed paths from Git or a plain file.

State clearly: diff2test does not run these commands.

## Usage

- synopsis;
- stdin example;
- file example;
- `human` and `names` formats;
- explanation flag;
- configuration option;
- exit status table.

## Safety behavior

Table:

- `SUBSET_SELECTED`;
- `FULL_SUITE_SELECTED`;
- `FULL_SUITE_REQUIRED`.

Show one missing-evidence output.

## Limitations

List real unsupported cases:

- wrappers/interpreters;
- generated/custom-command chains;
- stale metadata residual risk;
- platform/dependency formats;
- multiple configurations if limited;
- unknown paths cause fallback;
- does not run tests.

## Zero-dependency design

- one implementation file;
- no packages/vendored code;
- link to `STDLIB.md`;
- link to dependency proof;
- disclose external metadata as input;
- explicitly separate permitted CMake build/input-generation tooling from runtime dependencies.

## Verification

- test command;
- fixture scenarios;
- dependency inspection;
- reproducible-build result if achieved.

## Demo

- five-minute video link;
- short scenario list.

## Project structure

Show actual concise tree. Explain one source file plus allowed tests/docs/fixtures.

## Design tradeoffs

- why safe over minimal;
- why exact mapping;
- why no CMake-language parser;
- why Linux-first.

## License

OSI-approved license selected during hackathon.

## Acknowledgements and evidence

- official CMake/CTest/GCC docs;
- organizer clarification summary, including the explicit build-tool ruling;
- no copied code.

## Final README checklist

- [ ] All commands tested from clean checkout
- [ ] No placeholder remains
- [ ] No claim lacks a test/demo
- [ ] Runtime/build tools distinguished
- [ ] CMake described precisely as permitted build/input-generation tooling, not a runtime dependency
- [ ] External metadata generation disclosed
- [ ] Exit statuses match binary
- [ ] Limits honest
- [ ] Demo link public
