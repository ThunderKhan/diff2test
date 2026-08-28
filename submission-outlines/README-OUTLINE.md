# README.md Submission Outline

Use this outline during the hackathon. Replace every placeholder from the actual implementation; do not publish aspirational claims.

## Header

- TestImpact++
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
- one exact clean build command;
- produced artifact path;
- optional test command.

## Generate inputs externally

Document commands for:

- File API query before CMake configure;
- configure/build that produces `.d`;
- `ctest --show-only=json-v1` export;
- changed paths from Git or a plain file.

State clearly: TestImpact++ does not run these commands.

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
- disclose external metadata as input.

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
- organizer clarification summary;
- no copied code.

## Final README checklist

- [ ] All commands tested from clean checkout
- [ ] No placeholder remains
- [ ] No claim lacks a test/demo
- [ ] Runtime/build tools distinguished
- [ ] External metadata generation disclosed
- [ ] Exit statuses match binary
- [ ] Limits honest
- [ ] Demo link public

