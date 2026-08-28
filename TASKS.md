# Dependency-Ordered Task Backlog

## Task rules

- All implementation tasks start after 28 August 2026, 18:00 UTC.
- Estimates are solo focused-work estimates, not promises.
- P0 tasks block submission. P1 improves quality. P2/P3 may be cut.
- Each task ends with a verification condition and a commit checkpoint.
- No optional work starts while a P0 release blocker exists.

## Phase 0 — Kickoff and feasibility gates

### T0.1 — Create official repository

- Priority: P0
- Estimate: 15 min
- Depends on: official kickoff
- Work: initialize public/private-then-public repository according to rules, add license and empty planning-aware README skeleton written during window.
- Accept: first commit timestamp is after kickoff; no pre-event project code copied.

### T0.2 — Metadata capture spike

- Priority: P0
- Estimate: 30 min
- Depends on: T0.1
- Work: create tiny CMake/CTest fixture, request File API codemodel, compile with dependency files, export CTest JSON.
- Accept: actual files are located and understood.

### T0.3 — Artifact-to-test mapping spike

- Priority: P0
- Estimate: 30 min
- Depends on: T0.2
- Work: compare normalized codemodel artifact path with CTest command token.
- Accept: direct mapping succeeds under chosen generator; otherwise decision gate invoked.

### T0.4 — `.d` mapping/completeness spike

- Priority: P0
- Estimate: 30 min
- Depends on: T0.2
- Work: map each compiled source/object to a dependency file; inspect escaping.
- Accept: choose `--dep-root` convention or explicit `--dep-list` fallback.

### T0.5 — Freeze MVP contracts

- Priority: P0
- Estimate: 15 min
- Depends on: T0.3, T0.4
- Work: update real repository contract notes with spike findings.
- Accept: no unresolved mapping assumption blocks parser work.

## Phase 1 — Foundation

### T1.1 — One-command build

- Priority: P0
- Estimate: 30 min
- Depends on: T0.5
- Work: C++20 build configuration producing `diff2test` from one source.
- Accept: clean build succeeds with warnings enabled.

### T1.2 — Minimal test harness

- Priority: P0
- Estimate: 45 min
- Depends on: T1.1
- Work: stdlib/dev-only harness with deterministic pass/fail result.
- Accept: one intentional passing and failing check behaves correctly; failing check then removed/fixed.

### T1.3 — Core diagnostics/results

- Priority: P0
- Estimate: 45 min
- Depends on: T1.1
- Work: status enum, diagnostic structure, exception boundary, input limits.
- Accept: main returns documented skeleton statuses without external process use.

### T1.4 — CLI parser

- Priority: P0
- Estimate: 90 min
- Depends on: T1.3
- Work: commands/options/help/validation.
- Accept: CLI cases CLI-01 through CLI-10 pass where applicable.

## Phase 2 — Parsers

### T2.1 — JSON tokenizer/parser core

- Priority: P0
- Estimate: 4 h
- Depends on: T1.3, T1.2
- Work: strict JSON value parser with source positions and limits.
- Accept: valid/malformed core suite passes; sanitizer smoke test clean if available.

### T2.2 — JSON typed accessors

- Priority: P0
- Estimate: 1 h
- Depends on: T2.1
- Work: required/optional member helpers with contextual errors.
- Accept: missing vs wrong-type diagnostics are distinct.

### T2.3 — Dependency parser

- Priority: P0
- Estimate: 3 h
- Depends on: T1.3, T1.2
- Work: continuation, rule separator, escape-aware tokens, diagnostics.
- Accept: DEP-01 through DEP-12 pass under frozen supported grammar.

### T2.4 — Path normalization

- Priority: P0
- Estimate: 2 h
- Depends on: T1.3
- Work: root classification, lexical normalization, containment, display keys.
- Accept: path suite passes, including escape rejection and deleted paths.

## Phase 3 — Metadata loaders

### T3.1 — CTest catalogue loader

- Priority: P0
- Estimate: 2 h
- Depends on: T2.1, T2.2, T2.4
- Work: kind/version/tests/commands validation.
- Accept: complete catalogue or `FULL_SUITE_REQUIRED`; never partial trust.

### T3.2 — File API index/codemodel loader

- Priority: P0
- Estimate: 3 h
- Depends on: T2.1, T2.2, T2.4
- Work: index selection, codemodel/configuration, safe references.
- Accept: configuration and path-traversal cases pass.

### T3.3 — Target-object loader

- Priority: P0
- Estimate: 3 h
- Depends on: T3.2
- Work: target ids/names/types/sources/artifacts/dependencies.
- Accept: missing reference, generated source, duplicate id cases classified safely.

### T3.4 — Dependency discovery/mapping

- Priority: P0
- Estimate: 3 h
- Depends on: T2.3, T2.4, T3.3
- Work: discover/list files, map rules to translation units, completeness audit.
- Accept: every in-scope TU either has trusted evidence or creates safety issue.

### T3.5 — Test artifact mapping

- Priority: P0
- Estimate: 2 h
- Depends on: T3.1, T3.3, T2.4
- Work: exact unique artifact-to-command mapping.
- Accept: direct match succeeds; wrapper/zero/multiple/basename cases force fallback.

## Phase 4 — Analysis vertical slice

### T4.1 — Graph construction

- Priority: P0
- Estimate: 2 h
- Depends on: T3.3, T3.4, T3.5
- Work: typed reverse indexes and evidence edges.
- Accept: counts/invariants verified on fixture.

### T4.2 — Direct source/header impact

- Priority: P0
- Estimate: 2 h
- Depends on: T4.1
- Work: changed path to TUs/targets/tests.
- Accept: scenarios A/B select expected tests.

### T4.3 — Target propagation

- Priority: P0
- Estimate: 2 h
- Depends on: T4.1
- Work: tested reverse target dependency traversal.
- Accept: chain/diamond/cycle tests pass.

### T4.4 — Explanation path

- Priority: P0
- Estimate: 2 h
- Depends on: T4.2, T4.3
- Work: predecessor/evidence capture and deterministic path choice.
- Accept: every selected test has a valid chain.

### T4.5 — Central safety evaluator

- Priority: P0
- Estimate: 2 h
- Depends on: T3.*, T4.*
- Work: issue categories to final outcomes; prevent early output.
- Accept: full mutation matrix never under-selects.

### T4.6 — Output formatters

- Priority: P0
- Estimate: 2 h
- Depends on: T4.4, T4.5, T1.4
- Work: human/names, stdout/stderr, deterministic sorting.
- Accept: golden output and exit-status tests pass.

## Phase 5 — Hardening

### T5.1 — Controlled integration fixture

- Priority: P0
- Estimate: 2 h
- Depends on: T4.6
- Work: expand spike fixture to three understandable tests and mutation copies.
- Accept: all demo scenarios reproducible from docs.

### T5.2 — Safety mutation suite

- Priority: P0
- Estimate: 3 h
- Depends on: T5.1, T4.5
- Work: remove/corrupt each evidence element.
- Accept: zero unsafe subset results.

### T5.3 — Robustness/resource limits

- Priority: P1
- Estimate: 2 h
- Depends on: parser completion
- Work: limits, large synthetic input, exception paths.
- Accept: bounded failure without crash/hang.

### T5.4 — Determinism sweep

- Priority: P0
- Estimate: 1 h
- Depends on: T4.6
- Work: repeated/shuffled runs and byte comparisons.
- Accept: identical outputs.

### T5.5 — Warning/sanitizer sweep

- Priority: P1
- Estimate: 1.5 h
- Depends on: feature freeze
- Work: compiler warnings, ASan/UBSan if available, manual ownership review.
- Accept: clean results or documented benign exception.

## Phase 6 — Documentation/submission

### T6.1 — README

- Priority: P0
- Estimate: 2 h
- Depends on: CLI freeze, fixture
- Accept: clean-checkout user can build, generate inputs externally, analyze, and understand limits.

### T6.2 — STDLIB log

- Priority: P0/P1 bonus
- Estimate: 1.5 h
- Depends on: implementation stable
- Accept: 10+ real substitutions, metadata disclosure, no inflated claims.

### T6.3 — Dependency proof

- Priority: P0
- Estimate: 1 h
- Depends on: clean release build
- Accept: build inspection and dynamic-link proof captured and explained.

### T6.4 — License/submission metadata

- Priority: P0
- Estimate: 30 min
- Depends on: repository
- Accept: OSI license, track, pitch, required files.

### T6.5 — Reproducible build attempt

- Priority: P3
- Estimate: 2 h cap
- Depends on: all P0 green
- Accept: two same-environment clean builds have identical published hashes; otherwise document attempt and stop.

### T6.6 — Demo rehearsal/recording

- Priority: P0
- Estimate: 4 h including retries/upload
- Depends on: T6.1–T6.4
- Accept: under-five-minute verified video and backup.

### T6.7 — Final clean-room verification

- Priority: P0
- Estimate: 1.5 h
- Depends on: feature freeze
- Accept: clone/build/test/demo commands all work; submission checklist complete.

## Milestone gates

| Deadline from kickoff | Must be true | If false |
|---:|---|---|
| H+2 | artifact and `.d` mapping proven | simplify input or pivot |
| H+10 | JSON + dep parser core passing | cut all optional formats |
| H+22 | metadata loaders working | require narrower fixture/config |
| H+34 | first end-to-end subset | stop adding parser features |
| H+44 | fallback mutation suite working | focus only safety |
| H+52 | MVP feature freeze | cut P1/P2/P3 |
| H+60 | docs/proof complete | no bonuses |
| H+66 | demo recorded | use buffer for upload/verification |
| H+70 | final submission ready | emergency corrections only |
