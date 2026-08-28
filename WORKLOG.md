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
- Subset selection is still intentionally locked until dependency-evidence completeness and changed-path impact are fully wired.

### Verification

A clean local CMake build with the project warning set enabled passed all currently registered focused tests:

- `json_tests`
- `dep_tests`
- `path_tests`
- `ctest_tests`
- `cmake_tests`

No runtime process-spawning or network functionality has been introduced.

### Next verification gate

1. Load the explicit `--dep-list` file and every referenced `.d` input.
2. Prove dependency evidence covers every supported compiled translation unit.
3. Map changed files/headers to affected translation units.
4. Propagate affected targets through reverse target dependencies.
5. Connect affected executable targets to mapped CTest tests.
6. Add mutation cases proving that missing/stale/ambiguous evidence cannot produce `SUBSET_SELECTED`.
7. Unlock subset output only after the central completeness audit passes.
