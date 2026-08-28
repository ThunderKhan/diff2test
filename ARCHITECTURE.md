# Architecture — diff2test

## 1. Architectural goal

Implement a readable, testable evidence analyzer in one C++20 source file without third-party runtime code or external process execution.

The architecture is a pipeline with an explicit safety ledger. Parsers produce data plus diagnostics/completeness state; the analyzer may emit a subset only after the ledger proves every required stage complete.

## 2. System context

```text
External developer workflow
  ├─ changed-path list
  ├─ CMake File API replies
  ├─ compiler dependency files
  └─ CTest JSON
             ↓ files/stdin only
        diff2test analyzer
             ↓
  selected names + explanations + status
             ↓
External workflow runs tests separately
```

diff2test has no process-control component by design.

## 3. One-file organization

Recommended top-to-bottom sections inside `diff2test.cpp`:

1. Standard headers and build metadata constants
2. `d2t::core` — identifiers, status/result types, limits
3. `d2t::diag` — source positions, diagnostics, rendering
4. `d2t::cli` — command/options parsing
5. `d2t::json` — tokenizer, parser, value representation/accessors
6. `d2t::dep` — Make-style dependency parser
7. `d2t::path` — normalization and root checks
8. `d2t::cmake` — File API/codemodel loader
9. `d2t::ctest` — CTest catalogue loader
10. `d2t::model` — graph and evidence records
11. `d2t::impact` — mapping/traversal/explanations
12. `d2t::safety` — completeness ledger and final outcome
13. `d2t::output` — human/names formatters
14. top-level orchestration and `main`

The exact namespace names may change, but the dependency direction must remain downward: parsing layers do not call output or orchestration layers.

## 4. Core domain types

Conceptual types, not implementation code:

### Identifiers

- `PathId`
- `TargetId`
- `TestId`
- `ArtifactId`
- `EvidenceId`

Compact numeric ids may reduce repeated strings and make graph traversal predictable. Human strings live in indexed records.

### Records

- `NormalizedPath`: display form, comparison key, root classification
- `TranslationUnit`: source path, object/dependency evidence
- `BuildTarget`: CMake id/name/type, sources, artifacts, dependencies
- `RegisteredTest`: name, command, mapped target or mapping failure
- `EvidenceEdge`: source node, destination node, evidence category/location
- `Diagnostic`: severity, code, message, optional source position
- `SafetyIssue`: category, subject, consequence

### Outcome

- state: subset / full-known / full-required / usage / internal
- selected test ids
- explanation paths
- sorted diagnostics

## 5. Parser design

### JSON

Use a recursive-descent parser with:

- byte cursor plus line/column;
- tokenizer/parser combined or separated according to clarity;
- bounded nesting;
- value representation supporting all JSON types;
- strict string escape and number grammar;
- object lookup helpers that distinguish missing member from wrong type;
- unknown fields ignored only after structural validity is established.

Avoid an overly clever zero-copy representation during the hackathon. A straightforward owning value tree is acceptable for bounded metadata sizes.

### Dependency files

Use a small state machine:

1. join physical lines using valid backslash-newline continuation;
2. locate unescaped rule separator;
3. tokenize targets/prerequisites with escape awareness;
4. normalize tokens;
5. return rules plus diagnostics.

Do not reuse shell tokenization assumptions; Make-style escaping is its own format.

## 6. Metadata loaders

### CMake loader responsibilities

- validate reply/index containment;
- select supported codemodel and configuration;
- follow referenced JSON files;
- load target records;
- preserve target ids as opaque strings;
- collect sources/artifacts/dependencies;
- mark generated or unsupported constructs;
- report missing references to safety ledger.

### CTest loader responsibilities

- validate kind/version;
- read all test names and commands;
- reject catalogue ambiguity;
- normalize first command token under build-root context;
- map only exact unique target artifacts;
- retain mapping failures for the safety ledger.

### Dependency loader responsibilities

- discover or receive `.d` files;
- parse each supported rule;
- connect rule target to translation unit/object under a tested policy;
- build reverse prerequisite index;
- detect missing/stale/duplicate ambiguity;
- preserve evidence filename.

## 7. Graph model

Maintain separate typed relationships rather than an untyped universal graph where possible:

- prerequisite path → translation units
- translation unit → owning targets
- target → dependent targets
- target artifact → registered tests

This keeps propagation rules explicit. Explanations can reconstruct a typed path from predecessor maps recorded during BFS.

## 8. Analysis algorithm

### Phase A: ingest and validate

Parse the test catalogue early, then all other inputs. Collect diagnostics and safety issues without emitting test names.

### Phase B: completeness audit

Check catalogue, codemodel, dependency coverage, mapping uniqueness, configuration, roots, and changed-path classification.

If any issue forbids subset selection, skip minimal traversal and construct the safe full outcome.

### Phase C: impact traversal

For each changed path:

1. mark directly matching translation unit(s);
2. use reverse `.d` index to mark translation units that depend on it;
3. mark owning targets;
4. traverse supported dependent-target direction;
5. mark tests mapped to affected target artifacts;
6. store predecessor/evidence for at least one path.

### Phase D: result audit

Ensure every output test belongs to the parsed catalogue, no duplicate exists, and deterministic sorting is applied. Re-check safety ledger before returning subset.

## 9. Target propagation direction

CMake target dependency direction requires a kickoff spike and fixtures. The implementation must name edges clearly—e.g., `target depends on dependency`—then construct reverse adjacency when asking “what becomes affected if dependency changes?” Do not traverse the wrong direction because of ambiguous variable names.

## 10. Path strategy

Use a single normalization boundary:

- convert separators to the platform-preferred lexical form;
- resolve `.`/`..` lexically;
- classify under project root, build root, external/system, or invalid;
- store a stable comparison key;
- retain display path relative to project/build root where possible.

Do not call filesystem canonicalization blindly: missing/deleted paths and symlinks complicate it. Any physical canonicalization must be optional and error-aware.

## 11. Safety ledger

Every loader returns useful partial data plus categorized safety issues. The final evaluator owns the policy that maps issues to outcomes. Parsers do not independently decide to output all tests.

Suggested categories:

- catalogue unavailable/untrusted;
- build graph unavailable/incomplete;
- dependency evidence incomplete/stale;
- test mapping incomplete;
- changed path unknown/unsafe;
- unsupported construct/version;
- resource limit;
- invocation error;
- internal invariant.

This centralization prevents a forgotten warning from accidentally permitting subset selection.

## 12. Error handling

- Use value-or-error results for expected failures.
- Exceptions may handle allocation/filesystem failures internally but must be caught at the orchestration boundary.
- Every caught unexpected exception becomes `INTERNAL_ERROR` and discards partial subset output.
- Diagnostics use stable codes, e.g., `JSON_UNTERMINATED_STRING`, `DEP_TRAILING_ESCAPE`, `SAFETY_UNKNOWN_CHANGED_PATH`.

## 13. Determinism

- Sort filesystem discoveries before parsing.
- Sort normalized records before assigning ids, or ensure output sorting is independent of id creation.
- Use lexical tie-breaks for BFS explanations.
- Never depend on directory iteration order or hash-table iteration order.
- Avoid timestamps in normal output and built-in version strings if attempting reproducible builds.

## 14. Security and resource limits

- Validate containment of referenced reply files.
- Reject traversal outside declared roots.
- Bound input size, nesting, node count, and diagnostic count.
- Do not interpret command strings, environment variables, or shell syntax.
- Do not execute metadata.
- Avoid quadratic concatenation for large strings and graphs.

## 15. Testability in a single file

Options to decide after kickoff:

1. black-box executable tests driven by a standard-library test harness/script allowed as dev-only tooling;
2. compile `diff2test.cpp` with a macro that excludes `main` and exposes internal test entry points;
3. built-in `self-test` mode, though this enlarges runtime surface and is not preferred.

Preferred approach: a simple separate test executable or shell harness created during the event, with no runtime dependency in the shipped artifact. The organizer allows separate tests.

## 16. Extension points after MVP

- `compile_commands.json` enrichment
- explicit manual mapping for wrapper tests
- per-test always-run policy
- multi-config support
- additional dependency formats
- JSON result output
- graph visualization/export
- freshness fingerprints

No extension point justifies weakening the current safety contract.
