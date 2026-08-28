# Product Requirements Document — TestImpact++

## 1. Product overview

TestImpact++ is a local C++20 command-line analyzer for CMake/CTest projects. It consumes a changed-file list and pre-generated build/test metadata, builds an evidence graph, selects tests affected by the changes, explains each selection, and safely requests the full suite when the evidence is insufficient.

## 2. Product objective

Reduce unnecessary C++ test execution without introducing silent false-negative selection caused by guessing or incomplete metadata.

## 3. Users

### Primary persona: C++ developer

- works in a CMake/CTest repository;
- wants fast local feedback or a smaller CI job;
- understands where build metadata lives;
- values explanations over opaque predictions.

### Secondary persona: CI/build engineer

- integrates command-line tools through files, stdin, stdout, stderr, and exit codes;
- needs deterministic output;
- needs conservative failure behavior;
- needs dependency and security claims to be auditable.

## 4. Jobs to be done

1. Given a list of changed paths, determine which registered tests are provably affected.
2. Explain the dependency path behind every selected test.
3. Detect incomplete or ambiguous evidence before it causes unsafe test omission.
4. Tell automation whether it may run a subset, must run all known tests, or must invoke its own full-suite workflow.
5. Do all analysis locally with one zero-runtime-dependency artifact.

## 5. User journey

1. The project's normal configure/build workflow produces metadata.
2. The user exports CTest test information to a file outside TestImpact++.
3. The user passes changed paths through stdin or a file.
4. TestImpact++ validates inputs and format versions.
5. It builds file, translation-unit, target, artifact, and test relationships.
6. It evaluates evidence completeness.
7. It emits either:
   - a justified subset with explanations;
   - the full known suite with a fallback reason; or
   - `FULL_SUITE_REQUIRED` when the suite cannot be enumerated.
8. The user's workflow executes tests separately.

## 6. Functional requirements

### FR-1: CLI

The program shall provide discoverable help, version information, validation errors, documented exit statuses, and an `analyze` command.

### FR-2: Changed-path ingestion

The program shall read newline-delimited changed paths from exactly one file or stdin source, reject NUL-containing lines, normalize supported paths relative to a declared project root, and handle duplicates deterministically.

### FR-3: JSON parsing

The program shall parse standard JSON required by CMake File API and CTest models without a third-party library. Malformed input shall produce file/line/column diagnostics and trigger the contractually safe outcome.

### FR-4: CMake metadata

The program shall read a supported File API reply index, select an explicit or unambiguous configuration, and load supported codemodel target objects.

### FR-5: Dependency files

The program shall parse supported Make-style `.d` files, including line continuation and escaped whitespace, and relate prerequisites to their compiled target/translation unit where that mapping is evidenced.

### FR-6: CTest catalogue

The program shall parse CTest JSON model major version 1 and retain registered test names, command arrays, and relevant properties needed for mapping or safety decisions.

### FR-7: Mapping

The program shall map a test to a CMake executable target only when the normalized command executable matches exactly one reported target artifact under the selected configuration.

### FR-8: Impact analysis

The program shall perform reverse graph traversal from changed paths to affected translation units and targets, then select mapped tests associated with affected targets.

### FR-9: Target dependency propagation

The program shall conservatively propagate changes through supported target dependency relationships. Unsupported relationship types encountered on an affected path shall cause fallback.

### FR-10: Explanation

For each selected test, the program shall provide at least one evidence path beginning with a changed path and ending with the test.

### FR-11: Safety evaluation

The program shall distinguish subset success, full-known-suite fallback, usage/input error, and `FULL_SUITE_REQUIRED` according to `SAFETY-CONTRACT.md`.

### FR-12: Determinism

For byte-identical inputs and options, the program shall order tests, diagnostics, and explanations deterministically.

### FR-13: No execution

The program shall not run selected tests or invoke any external program.

## 7. Non-functional requirements

### NFR-1: Zero dependency

The runtime artifact shall contain no third-party code or runtime dependency and shall use permitted C++/libc/POSIX facilities only.

### NFR-2: Single-file readability

All implementation code shall live in one source file while retaining clear internal sections, names, small functions, and explicit error paths.

### NFR-3: Performance

The MVP target is analysis of a small-to-medium project in under two seconds on the developer's machine after file data is available. Correctness and safety take priority over this goal.

### NFR-4: Memory

The tool should process expected hackathon/demo inputs comfortably on a 4 GB RAM machine. Input-size guards shall prevent pathological allocation.

### NFR-5: Diagnostics

Errors shall name the input, explain the defect, and state the safety consequence. Parser errors should include position information.

### NFR-6: Automation friendliness

Normal selected-test data goes to stdout. Diagnostics and fallback explanations go to stderr unless an explicit human output mode combines them. Exit statuses shall be stable.

### NFR-7: Security posture

Input is untrusted data. The tool shall not execute it, follow unsafe paths without validation, or permit unbounded nesting/size that trivially exhausts resources.

## 8. Safety requirements

1. Absence of an edge is treated as evidence only when the corresponding evidence source is known complete.
2. An unknown changed path triggers full-suite behavior.
3. A missing dependency file for an in-scope translation unit triggers full-suite behavior.
4. An unmapped registered test triggers full-suite behavior in MVP.
5. Unsupported metadata major versions trigger full-suite behavior or input error; never partial trust.
6. The output must visibly state when it represents fallback rather than precise selection.
7. No success status may be returned for an unsafe partial result.

## 9. Output modes

### Human mode

Shows changed paths, affected translation units/targets/tests, evidence paths, and safety status.

### Names mode

Emits one selected test name per line for shell/CI consumption. Status and reasons remain on stderr.

Machine JSON output is a stretch goal because it requires serialization in addition to parsing. It may be added only after the primary output is stable and tested.

## 10. Success metrics

### Submission-critical

- one-command clean build;
- zero third-party runtime dependencies proven;
- one implementation source file;
- selective result correct for all supported test cases;
- every incomplete-evidence test produces full-suite behavior;
- no external process invocation;
- deterministic output;
- five-minute demo completes successfully.

### Product-quality targets

- at least 25 focused automated cases;
- at least 5 malformed JSON cases;
- at least 5 `.d` escaping/continuation cases;
- at least 8 safety/fallback cases;
- one realistic integration fixture with at least three test executables and a shared transitive header;
- all claims and limitations documented.

## 11. Non-goals

- test execution or scheduling;
- arbitrary CMake-language evaluation;
- coverage-based or predictive selection;
- remote services or repositories;
- source-control operations;
- automatic metadata generation;
- complete cross-platform support in 72 hours;
- perfect minimality for unsupported project structures;
- replacement for CTest itself.

## 12. Acceptance criteria

### AC-1: Direct source change

Given complete supported metadata and a changed test source, only its mapped test and any conservatively dependent tests are selected, with an evidence path.

### AC-2: Transitive header change

Given a header appearing in multiple `.d` files, all tests reachable through those translation units/targets are selected.

### AC-3: Unrelated change

Given a mapped source change unrelated to tests, the result may be an empty subset only if all registered tests and dependency evidence are complete and the safety contract permits it.

### AC-4: Missing dependency evidence

Removing one required `.d` file selects all known tests and emits a fallback reason.

### AC-5: Missing test catalogue

Absent or unreadable CTest JSON emits `FULL_SUITE_REQUIRED`; it does not invent test names.

### AC-6: Wrapper test command

A test command whose executable is not an exact known target artifact triggers full-known-suite fallback.

### AC-7: Malformed JSON

Malformed metadata emits a positional diagnostic and the correct safe status.

### AC-8: No subprocess

Code review and dependency proof find no process-spawning path.

### AC-9: Repeated result

Two analyses of identical files produce identical stdout, stderr ordering, and status.

### AC-10: Judge flow

A clean checkout builds and demonstrates selective analysis plus fallback in under five minutes using documented commands.

## 13. Release policy for the hackathon

Freeze new functionality once all P0 acceptance criteria pass. After freeze, only correctness fixes, tests, docs, demo work, and dependency proof are allowed unless a P0 requirement is impossible and the MVP contract is explicitly revised.

