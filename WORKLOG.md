# Hackathon Work Log

All implementation work in this log is after the official Zero Dependency Hackathon kickoff: 28 August 2026, 18:00 UTC (23:30 IST).

## 2026-08-28

### Kickoff and feasibility

- Re-read the organizer clarifications and project authority order before implementation.
- Started the in-window implementation baseline.
- Added a one-command C++20 build with a single implementation source: `diff2test.cpp`.
- Added the CLI/safety spine with documented outcome exit codes and no subprocess capability.
- Added a controlled CMake/CTest fixture and performed the metadata-mapping spike.
- Confirmed that CTest executable paths can be mapped exactly to CMake File API executable artifacts after build-root resolution.
- Confirmed CMake target dependency direction and the need for reverse adjacency during impact traversal.
- Observed unrelated `.d` files such as `link.d`; froze the MVP to explicit `--dep-list` input rather than unsafe recursive `.d` discovery.
- Corrected File API query placement: the query marker belongs under the build tree before configure.

### Foundation parsers

- Implemented a strict zero-dependency JSON parser with UTF-8 validation, JSON escapes, surrogate-pair handling, number grammar, duplicate-key rejection, positional diagnostics, and resource limits.
- Added focused JSON parser tests for valid and malformed cases.
- Implemented the supported GCC/Clang Make-style `.d` parser with continuation handling, escaping, comments, CRLF support, deterministic prerequisite deduplication, and malformed-input rejection.
- Added focused dependency parser tests.
- Implemented lexical path normalization and project/build-root containment checks without blindly canonicalizing nonexistent/deleted paths.
- Added path safety tests, including prefix-confusion and root-escape cases.

### Metadata loaders

- Implemented CTest `ctestInfo` JSON major-version-1 catalogue loading.
- Catalogue loading rejects malformed JSON, unsupported major versions, empty/duplicate test names, missing commands, and non-string command entries.
- Implemented CMake File API reply index/codemodel loading for codemodel major version 2.
- Added deterministic configuration selection and safe referenced-file containment checks.
- Implemented target-object loading for ids, names, types, sources, artifacts, and target dependencies.
- Implemented exact normalized CTest executable-artifact mapping; basename-only matching remains forbidden.
- Wired real safety outcomes into `analyze`:
  - missing/untrusted CTest catalogue -> `FULL_SUITE_REQUIRED`;
  - bad/ambiguous CMake metadata -> `FULL_SUITE_SELECTED`;
  - non-unique/unmapped test executable -> `FULL_SUITE_SELECTED`.

### End-to-end impact slice

- Implemented explicit `--dep-list` loading; diff2test still never invokes the compiler, CMake, CTest, Git, or a shell.
- Each listed dependency file is parsed as untrusted Make-style dependency evidence.
- Dependency coverage is tracked per `(CMake target, compiled source)` pair, preventing a source compiled into several targets from being treated as covered after only one `.d` file.
- The controlled Unix Makefiles MVP maps a dependency-rule target through `CMakeFiles/<target>.dir/`; unmatched or ambiguous layouts trigger full-suite fallback rather than guessing.
- Implemented changed-path ingestion from a file or stdin.
- Unknown changed paths, build-configuration changes, malformed dependency evidence, duplicate dependency evidence, missing translation-unit coverage, root mismatches, or unknown target edges widen to the full known suite.
- Implemented changed path -> translation unit -> owning target -> reverse target dependency propagation -> mapped CTest test selection.
- `SUBSET_SELECTED` is now reachable only after the CTest catalogue, codemodel, exact test mapping, dependency coverage, changed-path classification, and target graph all pass their completeness checks.
- Added `impact_tests` with a successful subset case and mutations for missing `.d` coverage, unknown changed paths, and a missing CTest catalogue.

### Verification

A clean local CMake build using `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` passed all six registered test executables:

- `json_tests`
- `dep_tests`
- `path_tests`
- `ctest_tests`
- `cmake_tests`
- `impact_tests`

Result: 6/6 tests passed, with no compiler warnings observed in the final local sweep.

No runtime process-spawning or network functionality has been introduced.

### Remaining release blockers

- Add a broader safety-mutation matrix: malformed listed `.d`, duplicate/ambiguous mappings, altered configuration metadata, wrapper-style CTest commands, and more graph mutations.
- Add detectable `.d` staleness/freshness handling or explicitly narrow the safety contract before release; stale metadata must never be advertised as proven fresh.
- Replace generic explanation text with concrete changed path / translation-unit / target / evidence-file paths.
- Exercise the real controlled fixture end to end using metadata generated externally by CMake/compiler/CTest, not only the synthetic integration fixture.
- Document the current Unix Makefiles dependency-target layout boundary clearly in final README/STDLIB/limitations.
