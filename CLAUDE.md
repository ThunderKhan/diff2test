# Claude Code Project Instructions — Activate Only at Kickoff

## Temporal constraint

Do not create or modify project implementation, tests, fixtures, build scripts, or repository configuration before 28 August 2026 at 18:00 UTC. Before that instant, you may read and critique planning documents only.

Once kickoff is confirmed, all project code must be newly written during the 72-hour window. Never copy pre-existing project code, vendored libraries, or code from an earlier private implementation.

## Project

TestImpact++ is a C++20 CLI that consumes changed paths and pre-generated CMake/compiler/CTest metadata, selects the smallest test subset justified by complete supported evidence, explains each selection, and requests the full suite whenever evidence is incomplete.

## Authority order

1. Official rules and `ORGANIZER-CLARIFICATIONS.md`
2. `SAFETY-CONTRACT.md`
3. `MVP.md`
4. `INPUT-SPEC.md` and `CLI-CONTRACT.md`
5. `ARCHITECTURE.md`
6. `TEST-PLAN.md`
7. `TASKS.md` and `KICKOFF-RUNBOOK.md`
8. `PRD.md`

If documents conflict, stop and report the conflict. Do not silently choose the less conservative rule.

## Absolute runtime prohibitions

The submitted program must never launch:

- Git;
- CMake;
- CTest;
- compilers;
- Python or interpreters;
- shells;
- any external executable or service.

Do not introduce process-spawn APIs, shell commands, third-party runtime libraries, package installation, network access, telemetry, or dynamic plugins.

Build tools and dev-only tests are separate from runtime and must be disclosed as required.

## Single-file requirement

All implementation belongs in one `testimpact.cpp`. No implementation headers, modules, generated source, `src/` tree, embedded third-party code, or duplicated alternate implementation.

Separate tests, fixtures, build scripts, README, STDLIB, and proof files are allowed.

Keep the one file readable using ordered internal namespaces/sections, short functions, explicit domain types, and one-way dependencies. Do not minify or compress code for the bonus.

## Safety law

False positives are acceptable; false negatives are not. A subset may be emitted only after the central safety audit establishes completeness. Missing, malformed, stale, unsupported, unknown, or ambiguous evidence must expand to all known tests or `FULL_SUITE_REQUIRED` when the catalogue is unavailable.

Never:

- treat missing data as an empty relationship;
- guess target/test mappings;
- use basename-only matching;
- ignore unknown changed paths;
- continue through unsupported major versions;
- output tentative test names before safety status is final;
- label fallback as precise selection.

## Scope

Implement P0 tasks first. Current MVP is Linux, C++20, File API codemodel v2, GCC/Clang Make-style `.d`, CTest JSON major v1, one explicit configuration, and exact executable-artifact mapping.

Do not add cross-platform formats, raw Git parsing, test execution, CMake-language interpretation, coverage/history/ML, arbitrary wrappers, or services unless every P0 task is complete and the user explicitly approves a contract revision.

## Work protocol

For each task:

1. cite the relevant contract/acceptance cases;
2. inspect current files and tests;
3. state a bounded plan;
4. implement only that scope;
5. add or update tests, including a negative/safety case;
6. run the narrow tests, then relevant full tests;
7. summarize changed files, commands run, and remaining risks;
8. do not declare completion when tests are skipped or failing.

Ask before materially changing CLI, exit codes, input formats, safety outcomes, supported platform, primary bonus, or file layout.

## Coding rules

- C++20 and permitted standard library/libc/POSIX only.
- Prefer ISO C++ where adequate.
- Compile with strong warnings.
- No undefined-behavior-dependent tricks.
- Bound parser nesting/input/resource use.
- Catch unexpected exceptions at top level and discard partial subset output.
- Use clear result/error types for expected failures.
- Keep target dependency direction explicit in names.
- Sort all user-visible output deterministically.
- Do not depend on filesystem or hash iteration order.
- Preserve file/line/column diagnostics for parsers.
- Treat metadata paths as untrusted and enforce root containment.
- Do not optimize until correctness tests pass.

## Testing rules

- Tests are written after kickoff.
- Every P0 behavior needs a test.
- Every parser feature needs malformed counterparts.
- Every successful selective fixture must be mutation-tested by removing evidence.
- Any mutation that still returns subset success is a release blocker.
- Run determinism comparisons.
- Sanitizers may be used as dev tooling if available and disclosed appropriately.

## Documentation rules

- README claims must match passing tests.
- `STDLIB.md` must disclose pre-generated metadata and each genuine stdlib substitution.
- Explain that users generate metadata externally and that TestImpact++ never invokes those tools.
- State Linux/compiler/generator boundaries honestly.
- Dependency proof must make C++ system runtime linkage understandable.
- Never claim mathematically complete selection for stale/adversarial metadata.

## AI integrity

AI assistance is permitted, but the human participant must be able to explain and defend the result. Do not bulk-generate unrelated features or hide uncertainty. If a format detail is uncertain, consult primary official documentation and label the conclusion.

## Definition of done

A task is done only when its acceptance condition passes, contracts remain consistent, safety does not regress, and the relevant command output has been reviewed. The project is done only after clean checkout/build/test, selective and fallback demos, dependency proof, and submission documents all pass.

