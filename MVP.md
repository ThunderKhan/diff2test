# Minimum Viable Product — diff2test

## MVP statement

Within 72 hours, ship one readable C++20 implementation file that safely selects affected CTest tests for a deliberately supported class of CMake projects using changed paths, File API codemodel data, GCC/Clang-style dependency files, and pre-generated CTest JSON—without launching external programs.

## Supported environment

- Linux
- GCC or Clang producing Make-style `.d` files
- CMake project with File API codemodel v2 replies
- one explicitly chosen or unambiguous build configuration
- CTest `ctestInfo` JSON major version 1
- tests whose command executable directly matches a CMake executable artifact
- UTF-8/ASCII project paths without embedded newline or NUL

## Required inputs

1. Project root
2. Build root
3. Changed paths through a file or stdin
4. CMake File API reply directory
5. CTest JSON file
6. Either:
   - dependency root scanned for supported `.d` files; or
   - explicit dependency-file list if discovery proves unreliable during the kickoff spike
7. Optional explicit configuration name when more than one exists

## Required capabilities

### P0-1: CLI foundation

- `analyze`, `--help`, `--version`
- required-option validation
- mutually exclusive changed-file sources
- deterministic names output and readable human output
- stable exit statuses

### P0-2: JSON parser

- objects, arrays, strings, numbers, booleans, null
- JSON escapes including `\uXXXX` handling sufficient for valid input
- nesting-depth and input-size guards
- file/line/column diagnostics
- no third-party parser

### P0-3: CMake File API loader

- locate/select a supported reply index
- follow codemodel reference from the index
- select one configuration
- load target JSON objects referenced by codemodel
- collect target id/name/type, sources, artifacts, and dependencies required by MVP

### P0-4: Dependency-file loader

- Make rule separator
- backslash-newline continuation
- escaped whitespace/backslash/hash/colon handling needed for GCC/Clang outputs
- multiple prerequisites
- deterministic normalization
- explicit malformed/incomplete result

### P0-5: CTest loader and mapping

- validate `kind` and major version
- collect every test name and command
- exact normalized executable-artifact mapping
- treat zero/multiple matches and wrappers as unsafe

### P0-6: Graph and selection

- map changed translation units directly
- reverse-map changed headers through `.d` prerequisites
- map translation units to owning targets
- propagate supported target impact
- map affected executable targets to tests
- retain at least one explanation path per selected test

### P0-7: Safety fallback

- audit required evidence before subset output
- full known test list when catalogue exists but analysis is unsafe
- `FULL_SUITE_REQUIRED` when catalogue cannot be enumerated
- state exact fallback reason

### P0-8: Verification and submission

- automated parser/graph/safety tests
- realistic integration fixture created after kickoff
- one-command build
- dependency proof
- `README.md` and `STDLIB.md`
- five-minute demo recording

## MVP data model

Conceptual nodes:

- normalized path
- translation unit
- build target
- target artifact
- registered test

Conceptual relationships:

- dependency-file prerequisite → translation unit
- translation unit → owning target
- target → dependent target, only for modeled relationship
- executable target artifact → registered test command

Every relationship stores evidence origin so an explanation can cite the source file or metadata object that justified it.

## MVP safety policy

Subset selection is permitted only when all are true:

- changed input is syntactically valid and non-ambiguous;
- every changed path is known or explicitly classified as safe by a narrow rule;
- supported metadata versions are present;
- configuration selection is unambiguous;
- every relevant target has adequate dependency evidence;
- every registered test maps exactly once;
- no unsupported affected relationship is encountered;
- all normalized paths remain inside declared roots or are intentionally accepted system paths that do not affect project mapping.

Otherwise select the full known suite or emit `FULL_SUITE_REQUIRED`.

## Controlled demo project requirements

Create only after kickoff. It should contain:

- one shared core library;
- two independent feature libraries or executables;
- three direct CTest executable tests;
- a header used by exactly one test path;
- a shared header used by multiple paths;
- one unrelated source;
- generated complete metadata;
- a reproducible way to remove/corrupt one metadata file for fallback demonstration.

The demo should prove:

1. narrow direct selection;
2. transitive shared-header selection;
3. deterministic explanation;
4. full-suite fallback when evidence is removed.

## Quality floor

- No uncaught exception crosses `main`.
- No silent parser recovery after malformed structural input.
- No unordered-container iteration leaks into output ordering.
- No status named “success” when fallback occurred.
- No unsupported case is guessed through.
- No external command invocation exists.
- Every P0 behavior has at least one test.

## Scope cuts, in order

If behind schedule, cut in this order:

1. JSON output mode
2. ANSI color
3. `compile_commands.json` support
4. automatic `.d` discovery; require explicit list/root convention
5. target dependency propagation beyond directly owning test targets
6. multiple explanation paths; keep one
7. configurable ignore patterns
8. multi-configuration convenience; require exact configuration
9. POSIX-specific enhancements
10. reproducible-build and Package Killer bonus attempts

Never cut:

- conservative fallback;
- complete test catalogue handling;
- parser diagnostics;
- no-subprocess rule;
- direct and transitive header selection for the supported fixture;
- dependency proof and honest limitations.

## Definition of MVP complete

MVP is complete when:

- all P0 tests pass from a clean build;
- supported demo input yields a smaller correct subset;
- every designed evidence gap yields the full safe outcome;
- output and statuses match the frozen CLI/safety contracts;
- a reviewer can read the implementation source top to bottom;
- docs explain how metadata is generated externally;
- no third-party runtime dependency or vendored code exists;
- the five-minute demo has been rehearsed at least twice.
