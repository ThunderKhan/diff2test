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
- Wired safety outcomes into `analyze`:
  - missing/untrusted CTest catalogue -> `FULL_SUITE_REQUIRED`;
  - bad/ambiguous CMake metadata -> `FULL_SUITE_SELECTED`;
  - non-unique/unmapped test executable -> `FULL_SUITE_SELECTED`.

### End-to-end impact slice

- Implemented explicit `--dep-list` loading; diff2test still never invokes the compiler, CMake, CTest, Git, Python, or a shell.
- Each listed dependency file is parsed as untrusted Make-style dependency evidence.
- Dependency coverage is tracked per `(CMake target, compiled source)` pair, preventing a source compiled into several targets from being treated as covered after only one `.d` file.
- The controlled Unix Makefiles MVP maps a dependency-rule target through `CMakeFiles/<target>.dir/`; unmatched or ambiguous layouts trigger full-suite fallback rather than guessing.
- Implemented changed-path ingestion from a file or stdin.
- Unknown changed paths, build-configuration changes, malformed dependency evidence, duplicate dependency evidence, missing translation-unit coverage, root mismatches, or unknown target edges widen to the full known suite.
- Implemented changed path -> translation unit -> owning target -> reverse target dependency propagation -> mapped CTest test selection.
- `SUBSET_SELECTED` is reachable only after the CTest catalogue, codemodel, exact test mapping, dependency coverage, changed-path classification, and target graph pass their completeness checks.

### Safety hardening

- Expanded `impact_tests` into an adversarial mutation matrix covering:
  - missing dependency coverage;
  - malformed dependency files;
  - missing dependency list;
  - duplicate dependency evidence;
  - unknown changed paths;
  - changed CMake configuration files;
  - malformed, unsupported, and missing CTest catalogues;
  - wrapper-style CTest commands;
  - malformed and ambiguous CMake indexes;
  - missing target objects;
  - declared-root mismatch;
  - unknown target dependency edges.
- Added a detectable-staleness audit for project prerequisites in `.d` files. A project prerequisite newer than its `.d` file, or an unreadable required timestamp, prevents subset output.
- Added a stale-evidence mutation test that deliberately makes a `.d` file older than a project header and verifies `FULL_SUITE_SELECTED`.
- Preserved the documented freshness caveat: passing timestamp checks means no detectable staleness under this policy, not cryptographic proof that metadata matches source contents.
- Replaced generic explanation text with concrete evidence chains containing the changed path, dependency file, translation unit, owning target, propagated dependent targets where applicable, and registered test.

### CI and real generated-metadata verification

- Added a public GitHub Actions workflow using only runner-provided Git/CMake/compiler/CTest as development/build tooling. No GitHub Action package is required for checkout; the workflow performs a plain Git fetch of the exact commit.
- CI builds the single runtime source and the registered test executables with the repository warning configuration, then runs the full CTest suite.
- CI audits `diff2test.cpp` for runtime process-spawn APIs and inspects dynamic linkage with `ldd`; neither operation is performed by the diff2test runtime artifact.
- Added a real controlled-fixture integration gate:
  - creates the CMake File API query externally;
  - configures/builds the fixture externally;
  - exports CTest `json-v1` externally;
  - collects real compiler `.o.d` files into an explicit dependency list;
  - runs diff2test on `include/alpha.hpp` and requires exactly `AlphaTest` with exit 0;
  - removes the `alpha` dependency evidence and requires all three tests (`AlphaTest`, `BetaTest`, `CoreTest`) with exit 10.
- GitHub Actions run `33205121938` completed successfully: configure, build, tests, metadata generation, selective analysis, missing-evidence fallback, subprocess audit, and dynamic-link inspection all passed.

## 2026-08-29

### Determinism and graph-shape hardening

- Added `hardening_tests`, bringing the registered CTest test executables to seven.
- Built a synthetic target graph that simultaneously exercises:
  - a dependency chain deeper than one edge;
  - a diamond (`core -> left/right -> join`);
  - a target cycle guarded by visited-set traversal;
  - a separate unaffected test branch that must remain omitted.
- Verified that a change to a header used by the graph origin selects exactly the deep affected test and does not select the unrelated test.
- Verified the explanation path remains concrete through the graph: changed path, dependency file, translation unit, owning target, deterministic propagated target chain, registered test.
- Reversed CMake target-reference order, reversed explicit `.d` list order, reversed CTest catalogue order, and duplicated the changed path; the selected test set and explanation map remained identical.
- Repeated the same analysis and compared the complete in-memory result for deterministic equality.
- Added JSON resource-boundary tests for:
  - maximum input byte count;
  - maximum string byte count;
  - exact-boundary acceptance;
  - the pre-existing nesting limit.
- A first CI attempt exposed an unqualified test-helper collision with `std::quoted`; renamed the helper and kept the warning-clean build requirement rather than suppressing the compiler error.
- GitHub Actions run `33232555766` then completed successfully with the graph/determinism hardening suite enabled.

### Byte-stable CLI verification

- Added a real-fixture CI step that captures both stdout and stderr from `--format human --explain`.
- Re-runs analysis with reversed dependency-list ordering and duplicate changed-path input and requires byte-for-byte identical stdout and stderr using `cmp`.
- Repeats the same analysis 20 times and requires byte-for-byte identical stdout and stderr on every run.
- GitHub Actions run `33232592962` completed successfully, including the byte-stability gate, real selective analysis, real missing-evidence fallback, subprocess audit, and linkage inspection.

### Sanitizer hardening

- Added a separate dev-only CI job configured with GCC AddressSanitizer and UndefinedBehaviorSanitizer.
- Sanitizers are used only to compile/run the test binaries in CI; they are not part of the normal shipped `diff2test` executable or its runtime requirements.
- The sanitizer job builds the same test suite with `-fsanitize=address,undefined`, leak detection, halt-on-error behavior, and stack traces for undefined behavior.
- GitHub Actions run `33232624663` completed successfully: both the normal verification job and the sanitizer job passed, including all seven test executables under ASan/UBSan.

### Current verified state

- Single runtime implementation source: `diff2test.cpp`.
- Zero third-party runtime code introduced.
- No runtime subprocess or network capability introduced.
- Parser, loader, impact, mutation, stale-evidence, graph-shape, deterministic-result, and resource-boundary tests are registered in CI.
- Real externally generated CMake/compiler/CTest metadata produces the intended narrow selection.
- Real removal of dependency evidence produces the intended full-known-suite fallback.
- CLI human/explain output is byte-stable under reordered evidence and across 20 repeated real-fixture analyses.
- All seven registered test executables pass under AddressSanitizer and UndefinedBehaviorSanitizer in dev-only CI.
- Safety and input contracts match the implemented freshness and generator-layout boundaries.

### Next work

- Replace the skeletal root README with verified usage/build/limits/exit-code documentation.
- Produce the final `STDLIB.md` from genuine implemented substitutions only.
- Produce dependency-proof documentation/raw evidence and final submission metadata.
- Consider only narrowly justified remaining robustness work; do not expand platform/generator scope before submission quality is strong.
- Rehearse the five-minute demo from a clean checkout after documentation is frozen.
