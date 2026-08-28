# Final Feasibility Review

## Verdict

**Green with a tightly controlled MVP.**

The confirmed rules allow TestImpact++ to analyze pre-generated metadata without launching the tools that produced it. Official formats expose enough information to construct a useful evidence pipeline for common CMake/CTest projects. The central difficulty is not parsing alone; it is proving that a CTest command maps to a build target and proving that dependency evidence is complete. The conservative fallback contract contains these risks.

The project becomes Yellow or Red if scope expands to arbitrary CMake interpretation, every compiler/platform, test wrappers, generated-source chains, or history-based prediction.

## Confirmed rule boundary

### Permitted

- C++20 standard library, libc, and POSIX
- Filesystem and stdin input
- Parsing CMake File API reply JSON
- Parsing compiler `.d` dependency files
- Parsing CTest `json-v1` output
- CMake as build tooling
- Separate tests, docs, fixtures, and build scripts
- One implementation file for the Single File bonus

### Prohibited at runtime

- Launching Git, CMake, CTest, a compiler, Python, or a shell
- Relying on a third-party service
- Loading or vendoring a third-party runtime library
- Quietly treating missing metadata as proof that no dependency exists

## Evidence pipeline

| Stage | Required input | Produced information | Primary uncertainty | Safe response |
|---|---|---|---|---|
| Changed paths | newline-delimited file/stdin | normalized changed nodes | rename syntax, outside-root path, empty input | reject invocation or full suite, depending on cause |
| File API index | newest supported `index-*.json` | references to codemodel reply | no index, multiple ambiguous indexes, unsupported major | all known tests or `FULL_SUITE_REQUIRED` |
| Codemodel | configuration and target objects | target names, types, sources, artifacts, dependencies | multiple configs, missing artifact, generated sources | full suite unless selected configuration is unambiguous |
| `.d` files | dependency files for relevant translation units | header/source-to-object relationships | missing/stale/malformed/escaped tokens | full suite |
| Target graph | codemodel target relationships | impact propagation between targets | relationship semantics incomplete for custom logic | full suite when affected path crosses unsupported edge |
| CTest JSON | `ctestInfo` version 1 data | test names, commands, properties | wrapper commands, aliases, missing executable | full suite |
| Test mapping | target artifact paths plus test commands | executable target to registered tests | command does not equal a known artifact | full suite |
| Selection | complete graph and mappings | selected test names and reasons | any relevant evidence gap | full suite |

## Technical foundation

### CMake File API

CMake writes a reply index under `<build>/.cmake/api/v1/reply/`. The client must read the index first and follow its references. Codemodel v2 target objects provide logical target names, target types, sources, artifacts, and dependency relationships. This is enough to map project sources to build targets for supported configurations; it is not a general record of every runtime effect of arbitrary CMake code.

### Compiler dependency files

GCC-compatible `-MD`/`-MMD` output records Make-style prerequisites discovered during preprocessing. These files provide the transitive include evidence needed to connect a changed header to compiled translation units. They may be missing, stale, or syntactically non-trivial because of escaping and continuations; therefore completeness must be evaluated explicitly.

### CTest JSON

`ctest --show-only=json-v1` emits structured test information without running tests. TestImpact++ may consume a file created earlier by that command. The JSON includes test names and commands, but a command can invoke a wrapper, interpreter, launcher, or script rather than a target artifact. Only exact, normalized artifact matches are safe in the MVP.

## CMake-target-to-CTest mapping

### Reliably supported MVP case

A test's command executable token, after documented normalization, equals exactly one executable artifact path reported for exactly one CMake target in the selected configuration. That target can then map to the registered CTest test.

### Ambiguous or unsupported cases

- command invokes Python, shell, emulator, launcher, or wrapper;
- command uses an unresolved generator expression or environment-dependent path;
- one artifact path matches multiple targets;
- executable path is absent from codemodel;
- test executes multiple project binaries;
- fixtures/setup/cleanup create broader coupling;
- resource locks or required files imply relationships not modeled;
- test name exists but command data is malformed;
- multi-config metadata has no explicit selected configuration.

MVP response: select the full known suite. A later version could conservatively mark individual unmapped tests as always-selected, but that policy is not required for the first vertical slice.

## Graceful-degradation summary

| Condition | If test catalogue is readable | If catalogue is unavailable |
|---|---|---|
| Missing/malformed File API | output every test | `FULL_SUITE_REQUIRED` |
| Missing/malformed relevant `.d` | output every test | `FULL_SUITE_REQUIRED` |
| Unknown changed path | output every test | `FULL_SUITE_REQUIRED` |
| Unmapped test command | output every test | not applicable |
| Malformed CTest JSON | catalogue cannot be trusted | `FULL_SUITE_REQUIRED` |
| Empty legitimate change list | select none only with explicit `--allow-empty`; otherwise usage failure | usage failure |

Detailed stdout/stderr and exit behavior is defined in `SAFETY-CONTRACT.md` and `CLI-CONTRACT.md`.

## Minimum useful MVP

1. Linux, C++20, one implementation source file.
2. Human-readable CLI with `analyze`, `--help`, and `--version`.
3. Changed paths from newline-delimited file or stdin.
4. Hand-written JSON parser sufficient for standard JSON and positional diagnostics.
5. CMake File API codemodel v2 major support for one selected configuration.
6. GCC/Clang-style Make dependency files with whitespace escaping and line continuation.
7. CTest JSON model major version 1.
8. Exact target-artifact-to-test-command mapping.
9. Reverse impact traversal from changed files to translation units, targets, and tests.
10. Human explanation output plus deterministic line-oriented machine output.
11. Full-suite fallback for every incompleteness condition defined by the safety contract.
12. Tests that prove both selective success and conservative failure.

## Explicit exclusions

- Direct `CMakeLists.txt` interpretation
- Runtime tool execution
- Windows/MSVC dependency formats
- Apple/Xcode-specific modeling
- Compile database as a mandatory input
- Raw Git diff/rename parser
- Test history, coverage, probabilistic scoring, or ML
- Arbitrary wrapper introspection
- Generated-source/custom-command graph reconstruction
- Multi-configuration auto-detection
- Executing selected tests

## First-two-hours technical spikes

These are performed only after kickoff.

1. **Metadata capture spike:** create a tiny legal sample project, request File API codemodel, generate `.d` files, and export CTest JSON.
2. **Artifact mapping spike:** verify how the CTest command executable compares with the File API artifact path under the chosen generator.
3. **Dependency completeness spike:** confirm `.d` paths and escaping for a transitive header change.
4. **Configuration spike:** inspect single-config and, only if time permits, multi-config reply differences.
5. **Fallback spike:** remove one required file and confirm the intended test catalogue remains available for full selection.

Decision gate after spike 2: if exact artifact mapping is unreliable even in the controlled demo project, reduce MVP to an explicit target-to-test mapping input or pivot before implementing the full parser.

## Single-file design assessment

One implementation file is feasible if it is organized top-to-bottom into narrow internal sections:

1. domain/status types;
2. diagnostics and result helpers;
3. CLI parsing;
4. JSON lexer/parser;
5. dependency-file parser;
6. path normalization;
7. metadata loaders;
8. graph model;
9. impact/safety analysis;
10. output formatting;
11. `main` orchestration.

Avoid premature generic abstractions. Each section should expose a small set of functions and types inside internal namespaces. Tests may compile the file with a test-mode macro or exercise the executable; choose the simplest approach after kickoff without adding an implementation header.

## Bonus assessment

| Bonus | Decision | Evidence needed |
|---|---|---|
| Single File (+5) | Primary | one readable implementation source, no modules/src tree |
| STDLIB Log (+3) | Natural secondary | at least 10 real substitutions with rationale |
| Reproducible Build (+5) | Late stretch | same toolchain, two clean builds, identical hashes |
| Package Killer (+3) | Uncommitted | named package, download evidence, honest feature comparison |

## Risk register

| Risk | Probability | Impact | Earliest validation | Fallback |
|---|---:|---:|---|---|
| Test command does not map to target artifact | High | High | hour 1 spike | controlled demo + strict exact mapping; full suite otherwise |
| `.d` discovery is generator-specific | Medium | High | hour 1 spike | accept explicit `--dep-file-list` or narrow supported layout |
| JSON parser consumes too much time | Medium | High | first 8 hours | support full JSON grammar but only required fields; cut machine JSON output |
| Single file becomes unreadable | Medium | Medium | continuous review | strict sections, internal namespaces, short functions |
| Multi-config ambiguity | High | Medium | hour 2 | require `--configuration`; exclude auto-detection |
| Generated/custom-command paths | Medium | High | fixture inspection | full-suite fallback; document non-goal |
| Stale metadata produces unsafe result | Medium | Critical | safety review | optional freshness checks; conservatively require user attestation and document residual risk |
| Demo setup consumes time | Medium | High | first day | make demo project early after vertical slice |
| Reproducible build distracts from MVP | Medium | Medium | hour 54 gate | skip bonus |
| Package Killer claim is weak | High | Low | research checkpoint | do not claim it |

## Remaining conceptual risk: freshness

No parser can prove that supplied metadata corresponds to the current source tree solely from file contents. The MVP can compare timestamps when meaningful, but timestamps themselves are not a cryptographic guarantee. Documentation must state that users should regenerate metadata in their normal build step before analysis. If detectable evidence indicates staleness, fall back. Do not claim absolute soundness across stale or adversarial input.

## Recommendation

Proceed. Freeze the MVP around a controlled Linux/GCC-or-Clang/CMake/CTest workflow and exact artifact mapping. Treat every unsupported feature as a reason to widen selection, not a reason to improvise an inference.

