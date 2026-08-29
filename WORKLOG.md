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

- Expanded `impact_tests` into an adversarial mutation matrix covering missing/malformed/duplicate dependency evidence, unknown/build-config changes, CTest failures, wrapper commands, CMake ambiguity/root mismatch, missing target objects, and unknown target edges.
- Added a detectable-staleness audit for project prerequisites in `.d` files. A project prerequisite newer than its `.d` file, or an unreadable required timestamp, prevents subset output.
- Added a stale-evidence mutation test that deliberately makes a `.d` file older than a project header and verifies `FULL_SUITE_SELECTED`.
- Preserved the freshness caveat: passing timestamp checks means no detectable staleness under this policy, not cryptographic proof that metadata matches source contents.
- Replaced generic explanation text with concrete evidence chains containing changed path, dependency file, translation unit, owning target, propagated dependent targets, and registered test.

### CI and real generated-metadata verification

- Added a public GitHub Actions workflow using only runner-provided Git/CMake/compiler/CTest as development/build tooling. No GitHub Action package is required for checkout; the workflow performs a plain Git fetch of the exact commit.
- CI builds the single runtime source and registered test executables with the repository warning configuration, then runs the full CTest suite.
- CI audits `diff2test.cpp` for runtime process-spawn APIs and inspects dynamic linkage with `ldd`; neither operation is performed by the diff2test runtime artifact.
- Added a real controlled-fixture integration gate that externally creates the File API query, configures/builds the fixture, exports CTest `json-v1`, collects real compiler `.o.d` files into an explicit dependency list, verifies `include/alpha.hpp -> AlphaTest`, and verifies a missing-evidence full-suite fallback.
- GitHub Actions run `33205121938` completed successfully for the first end-to-end real metadata gate.

## 2026-08-29

### Determinism and graph-shape hardening

- Added `hardening_tests`, bringing the registered CTest test executables to seven.
- Built a synthetic target graph that simultaneously exercises a dependency chain deeper than one edge, a diamond (`core -> left/right -> join`), a target cycle guarded by visited-set traversal, and a separate unaffected test branch.
- Verified that a change to a header used by the graph origin selects exactly the deep affected test and does not select the unrelated test.
- Verified the explanation path remains concrete through the graph.
- Reversed CMake target-reference order, reversed explicit `.d` list order, reversed CTest catalogue order, and duplicated the changed path; the selected test set and explanation map remained identical.
- Repeated the same analysis and compared the complete in-memory result for deterministic equality.
- Added JSON resource-boundary tests for maximum input bytes, maximum string bytes, exact-boundary acceptance, and nesting limits.
- A first CI attempt exposed an unqualified test-helper collision with `std::quoted`; renamed the helper and kept the warning-clean requirement instead of suppressing the compiler error.
- GitHub Actions run `33232555766` completed successfully with the graph/determinism hardening suite enabled.

### Byte-stable CLI verification

- Added a real-fixture CI step that captures stdout/stderr from `--format human --explain`.
- Re-runs analysis with reversed dependency-list ordering and duplicate changed-path input and requires byte-for-byte identical stdout and stderr using `cmp`.
- Repeats the same analysis 20 times and requires byte-for-byte identical stdout/stderr on every run.
- GitHub Actions run `33232592962` completed successfully with the byte-stability gate.

### Sanitizer hardening

- Added a separate dev-only CI job configured with GCC AddressSanitizer and UndefinedBehaviorSanitizer.
- Sanitizers compile/run only test binaries in CI; they are not part of the normal shipped executable or runtime requirements.
- GitHub Actions run `33232624663` completed successfully with both normal and sanitizer jobs passing all seven test executables.

### Final contract/fixture verification

- Added `include/features_shared.hpp` to the real fixture and wired it into alpha/beta but not core.
- Public CI now verifies `include/features_shared.hpp` selects exactly `AlphaTest` and `BetaTest`.
- Added a 10,000-prerequisite `.d` parser stress case.
- Expanded CI to verify:
  - help/version basics;
  - missing/unknown commands;
  - unknown/repeated options and removed `--dep-root`;
  - stdin changed-path input matches file input;
  - empty changed input exits `64`;
  - missing catalogue exits `11` with no names;
  - narrow real selection;
  - shared-header real multi-test selection;
  - missing-evidence full-known-suite fallback;
  - byte-stable output across reordered evidence and 20 repetitions;
  - runtime process-spawn audit;
  - Linux dynamic-link inspection;
  - two independent clean Release builds compared byte-for-byte.
- GitHub Actions run `33233072308` completed successfully for all of the above and for the separate ASan/UBSan job.

### Reproducible build result

- Two independent same-runner/toolchain Release builds produced byte-identical `diff2test` binaries.
- Both had SHA-256:

```text
162a6bbf52034f0c468ab2c7c82853a449590530768e9ed6ddd82f1b7aabc903
```

- This is documented as a same-environment/toolchain reproducibility result only.

### Dependency proof result

The public Linux `ldd` output contained only expected system/toolchain runtime entries:

```text
linux-vdso.so.1
libstdc++.so.6
libgcc_s.so.1
libc.so.6
libm.so.6
/lib64/ld-linux-x86-64.so.2
```

No CMake library or third-party application runtime library appeared.

### Final documentation/submission pass

- Replaced the skeletal root README with verified build/input-generation/usage/safety/limitations/verification documentation.
- Added `STDLIB.md` with 10+ genuine standard-library/from-first-principles substitutions.
- Added `DEPENDENCY-PROOF.md` with exact runner/toolchain evidence, dynamic-link explanation, subprocess audit, metadata boundary, and reproducibility result.
- Added `SUBMISSION.md` with judge-facing project description/pitch/technical evidence.
- Added `DEMO-SCRIPT.md` with exact under-five-minute recording commands and narrative.
- Updated `DEMO-CONTRACT.md` so it references frozen commands rather than future placeholders.
- Updated `fixture/README.md` with narrow/shared/fallback/missing-catalogue scenarios.
- Updated `PROBLEM-BRIEF.md` to use the frozen `--build-root` + `--dep-list` CLI.
- Updated `CLI-CONTRACT.md` to match the verified binary rather than outdated planning examples, including the actual output/stderr/help/usage boundaries.
- Updated `MVP.md` to the final supported scope and separated engineering completion from human recording/submission actions.
- Added `FINAL-AUDIT.md` mapping every task phase to current evidence and explicitly scoping out unclaimed 100k/1M graph benchmarking.

### Current verified state

- Single runtime implementation source: `diff2test.cpp`.
- Zero third-party runtime code introduced.
- No runtime subprocess or network capability introduced.
- Parser, loader, impact, mutation, stale-evidence, graph-shape, deterministic-result, resource-boundary, large dependency-rule, CLI, real-fixture, sanitizer, and reproducibility checks are present.
- Real externally generated CMake/compiler/CTest metadata produces intended narrow and shared-header selections.
- Real evidence removal produces the intended full-known-suite fallback.
- Missing catalogue produces `FULL_SUITE_REQUIRED` without invented names.
- CLI human/explain output is byte-stable under reordered evidence and across 20 repeated real-fixture analyses.
- All seven registered test executables pass under AddressSanitizer and UndefinedBehaviorSanitizer in dev-only CI.
- Safety/input/CLI/MVP/demo documentation now matches the frozen implementation and tested boundaries.

### Human-only remaining submission actions

Repository engineering/documentation is feature-frozen. Remaining actions cannot be performed by CI or repository tooling:

1. rehearse `DEMO-SCRIPT.md` from a clean local checkout;
2. rehearse it a second time;
3. record the final under-five-minute demo;
4. inspect/upload the video and verify the public URL;
5. place the public video URL in the hackathon form (and optionally README);
6. use `SUBMISSION.md` in the final submission form;
7. verify the submitted repo/branch/form and submit before the event deadline.
