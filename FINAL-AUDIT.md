# Final MVP Audit — diff2test

This audit reconciles the planning/contracts against the current repository and public CI evidence. It separates completed engineering work from human-only submission actions.

## Executive status

**Core engine:** feature-frozen and verified for the supported MVP boundary.

**Repository/submission documentation:** prepared.

**Automated clean-room verification:** passing.

**Human-only remaining work:** rehearse the final demo twice, record/upload the video, insert its public URL in the hackathon form (and optionally README), and perform the final form submission.

## Bonus status

| Bonus | Status | Evidence |
|---|---|---|
| Single File (+5) | ✅ | runtime implementation is only `diff2test.cpp`; tests/docs/fixtures are separate development material |
| Reproducible Build (+5) | ✅ | two clean same-runner Release binaries compared byte-identical; SHA-256 recorded below |
| STDLIB Log (+3) | ✅ | `STDLIB.md` documents 10+ genuine substitutions actually present in the project |
| Package Killer (+3) | ✅ documented/claimed | `PACKAGE-KILLER.md` compares the supported workflow with RTS++ / Ekstazi++, a C++ regression-test-selection tool whose build requires LLVM, builds its own RTS library/pass, installs CMake package files, and identifies separate SHA-512 source; the claim is explicitly narrow rather than drop-in equivalence |

Potential documented bonus total: **+16**, subject to organizer/judge acceptance of the Package Killer comparison.

## Phase 0 — kickoff and feasibility

| Task | Status | Evidence |
|---|---|---|
| T0.1 official repository | ✅ | public repo created after official kickoff; work log records in-window implementation |
| T0.2 metadata capture spike | ✅ | controlled `fixture/` + `WORKLOG.md` metadata-spike record + public CI generated-metadata verification |
| T0.3 artifact-to-test mapping spike | ✅ | exact normalized CTest command ↔ File API artifact mapping verified |
| T0.4 `.d` mapping/completeness spike | ✅ | observed compiler `.o.d` plus unrelated `link.d`; chose explicit `--dep-list` |
| T0.5 freeze MVP contracts | ✅ | CLI/input/safety contracts updated to tested boundary |

## Phase 1 — foundation

| Task | Status | Evidence |
|---|---|---|
| T1.1 one-command C++20 build | ✅ | CMake Release/Debug builds pass in CI |
| T1.2 minimal test harness | ✅ | dependency-free C++ test executables registered with CTest |
| T1.3 diagnostics/results | ✅ | outcome enum, reasons, exception boundary, file/parser limits |
| T1.4 CLI parser | ✅ | public CI checks help/version, unknown/missing/repeated options, removed `--dep-root`, stdin/file equivalence, empty changed input |

## Phase 2 — parsers

| Task | Status | Evidence |
|---|---|---|
| T2.1 strict JSON parser | ✅ | full grammar subset used by metadata, UTF-8, escapes, surrogate pairs, malformed cases, limits |
| T2.2 typed JSON access | ✅ | required member/type validation throughout loaders |
| T2.3 Make-style dependency parser | ✅ | escapes, continuations, CRLF, comments, malformed cases; 10,000-prerequisite stress case |
| T2.4 path normalization | ✅ | lexical root classification/containment and escape/prefix-confusion tests |

## Phase 3 — metadata loaders

| Task | Status | Evidence |
|---|---|---|
| T3.1 CTest catalogue | ✅ | `ctestInfo` v1 validation; duplicate/malformed/unsupported cases |
| T3.2 File API index/codemodel | ✅ | codemodel v2, index/config selection, safe references |
| T3.3 target objects | ✅ | ids/names/types/sources/artifacts/dependencies; generated/invalid references fall back |
| T3.4 dependency mapping/completeness | ✅ | explicit list, `(target, source)` completeness, stale evidence checks |
| T3.5 test artifact mapping | ✅ | exact normalized executable-artifact match; wrapper/zero/ambiguous cases fall back |

## Phase 4 — analysis vertical slice

| Task | Status | Evidence |
|---|---|---|
| T4.1 graph construction | ✅ | prerequisite → TU → target + reverse target graph + test mapping |
| T4.2 direct source/header impact | ✅ | real narrow fixture selection and synthetic impact tests |
| T4.3 target propagation | ✅ | chain, diamond, cycle, unaffected branch hardening tests |
| T4.4 explanation path | ✅ | concrete changed path / dependency file / TU / target chain / registered test |
| T4.5 central safety evaluator | ✅ | broad mutation matrix; evidence gaps never remain narrow selections |
| T4.6 output formatters | ✅ | human/names, stdout/stderr split, sorted output, byte-stability checks |

## Phase 5 — hardening

| Task | Status | Evidence |
|---|---|---|
| T5.1 controlled integration fixture | ✅ | three tests with narrow/shared/fallback scenarios |
| T5.2 safety mutation suite | ✅ | malformed/missing/duplicate/stale/ambiguous/version/root/graph mutations |
| T5.3 robustness/resource limits | ✅ for MVP hardening | JSON byte/string/nesting boundaries, parser malformed corpus, 10k prerequisite stress case; no 1M-edge production benchmark claim |
| T5.4 determinism sweep | ✅ | reordered metadata/evidence + duplicates + 20 repeated real runs produce byte-identical stdout/stderr |
| T5.5 warnings/sanitizers | ✅ | warning-enabled build plus separate ASan/UBSan CI job passes all seven tests |

### Scope note on performance testing

The original test plan listed 10k/100k/1M synthetic graph sizes as a performance exploration target. That was never a P0 product requirement and the final MVP does **not** make a million-edge performance claim. The final hardening instead exercises a 10,000-prerequisite parser case plus realistic/synthetic graph correctness and sanitizer/determinism checks. This is an explicit scope boundary, not an unreported missing guarantee.

## Phase 6 — documentation/submission

| Task | Status | Evidence |
|---|---|---|
| T6.1 README | ✅ | final `README.md` with build, external metadata generation, CLI, safety, limits, verification |
| T6.2 STDLIB log | ✅ | `STDLIB.md` documents 10+ genuine substitutions and tooling/runtime boundary |
| T6.3 dependency proof | ✅ | `DEPENDENCY-PROOF.md`, real `ldd`, subprocess audit, clean builds |
| T6.4 license/submission metadata | ✅ | MIT license + `SUBMISSION.md` |
| T6.5 reproducible build attempt | ✅ | two clean same-runner Release binaries byte-identical; SHA-256 recorded |
| T6.6 demo rehearsal/recording | 🟡 human action | exact verified script frozen in `DEMO-SCRIPT.md`; recording/upload cannot be performed by repository automation |
| T6.7 final clean-room verification | ✅ engineering side | fresh GitHub runner checkout builds/tests/runs real fixture and proof checks successfully |

## Real integration evidence

GitHub Actions run `33233072308` completed successfully and verified:

- clean exact-commit checkout;
- warning-enabled build;
- all seven C++ tests;
- CLI contract basics;
- real CMake File API/compiler `.d`/CTest JSON generation outside `diff2test`;
- `include/alpha.hpp` -> exactly `AlphaTest`;
- stdin/file changed input equivalence;
- `include/features_shared.hpp` -> exactly `AlphaTest` + `BetaTest`;
- deterministic output under reordered evidence and 20 repeated executions;
- missing dependency evidence -> all known tests, exit 10;
- missing CTest catalogue -> zero invented names, exit 11;
- empty changed input -> exit 64;
- process-spawn source audit;
- `ldd` runtime inspection;
- two independent clean Release builds with byte-identical artifacts;
- separate ASan/UBSan build/test job.

Reproducible Release SHA-256 in that run:

```text
162a6bbf52034f0c468ab2c7c82853a449590530768e9ed6ddd82f1b7aabc903
```

## Single-file / zero-runtime-dependency audit

- runtime implementation: `diff2test.cpp` only;
- no implementation headers/modules/vendor tree;
- no runtime package manager;
- no network/service client;
- no process-spawn API found by CI audit;
- Linux dynamic links observed only to system/toolchain runtime libraries;
- CMake/CTest/Git/compiler are explicitly external build/input-generation tooling, never runtime subprocesses.

## Package Killer audit

The dedicated comparison is [`PACKAGE-KILLER.md`](PACKAGE-KILLER.md).

Primary target: **RTS++ / Ekstazi++** (`EngineeringSoftware/ekstazipp`). The upstream repository identifies itself as a C++ Regression Test Selection tool. Its CMake build uses `find_package(LLVM REQUIRED CONFIG)`, builds an LLVM pass and `ekstazi-lib`, and installs CMake package/export configuration. Its README separately identifies SHA-512 source from another repository.

`diff2test` replaces that dedicated RTS stack only for its tested CMake/CTest metadata workflow. The comparison explicitly preserves non-equivalence for arbitrary build systems, LLVM instrumentation workflows, broader test-framework integrations, Windows/MSVC, Ninja dependency databases, and other unsupported shapes.

No runtime implementation changes were made to manufacture this bonus claim; it documents the package/tool class the already-verified stdlib implementation can replace within its supported boundary.

## Known and intentional limitations

The final MVP does not claim support for:

- arbitrary CMake generators;
- Windows/MSVC dependency formats;
- Ninja `.ninja_deps`;
- recursive unsafe `.d` discovery;
- wrapper/interpreter test commands;
- generated/custom-command dependency chains;
- CMake-language interpretation;
- raw Git patch parsing;
- coverage/history/ML selection;
- absolute freshness proof;
- executing tests;
- cross-toolchain/cross-platform reproducible binaries.

Unsupported/ambiguous evidence widens to a safe outcome rather than being guessed through.

## Contract reconciliation notes

During final audit, `CLI-CONTRACT.md` was corrected to match the verified binary rather than preserving outdated planning examples. In particular:

- actual human explanation format is documented;
- successful names-mode subset does not promise a stderr summary;
- fallback stderr uses the symbolic status prefix actually emitted by the binary;
- built-in help is documented as concise, with detailed platform/exit guidance in README;
- `--verbose` is honestly documented as accepted but not producing additional MVP output;
- TTY detection and color flags are not claimed;
- root/configuration evidence problems are documented as analysis safety fallbacks where applicable rather than generic usage errors.

No safety invariant was weakened by this reconciliation.

## Human submission checklist

These are the remaining actions outside automated repository work:

- [ ] rehearse `DEMO-SCRIPT.md` once from a clean local checkout;
- [ ] rehearse it a second time;
- [ ] record the final demo under five minutes;
- [ ] inspect the recording for readable terminal output and accidental personal data;
- [ ] upload the video and verify the public link;
- [ ] put the final video URL into the hackathon form (and optionally README);
- [ ] paste/use `SUBMISSION.md` content in the submission form;
- [ ] verify the submitted repository branch/commit and form fields;
- [ ] submit before the event deadline.

Everything above those human-only actions is now repository-verifiable.
