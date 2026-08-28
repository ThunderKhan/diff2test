# Test Plan — TestImpact++

## 1. Test philosophy

The most important tests are not the cases where selection succeeds; they are the cases where one piece of evidence disappears and the program refuses to under-select.

Tests and fixtures described here must be implemented only after kickoff.

## 2. Test levels

### Unit-level

- CLI parser
- JSON parser/accessors
- dependency-file tokenizer/parser
- path normalization
- graph traversal
- outcome/safety evaluator
- deterministic formatting

### Component-level

- File API loader over synthetic reply files
- CTest catalogue loader and target mapping
- dependency completeness audit

### Integration-level

- controlled CMake/CTest project metadata through full CLI
- selective header/source changes
- deliberate corruption/removal and fallback

### Submission-level

- clean one-command build
- dependency inspection
- five-minute demo rehearsal
- repeated output/build comparisons

## 3. CLI cases

| ID | Case | Expected |
|---|---|---|
| CLI-01 | `--help` | usage text, exit 0 |
| CLI-02 | `--version` | stable version, exit 0 |
| CLI-03 | missing command | usage error 64 |
| CLI-04 | unknown option | names option, exit 64 |
| CLI-05 | repeated singleton option | exit 64 |
| CLI-06 | both dep input modes | exit 64 |
| CLI-07 | changed files from stdin | same result as file |
| CLI-08 | empty changed list | exit 64 under MVP policy |
| CLI-09 | names format | stdout contains names only |
| CLI-10 | fallback names format | all known names + nonzero safety status |

## 4. JSON cases

Minimum valid cases:

- empty object/array;
- nested objects/arrays;
- all scalar types;
- escape sequences;
- BMP Unicode escape;
- surrogate-pair policy tested/documented;
- integer/fraction/exponent numbers;
- whitespace variants.

Malformed cases:

- unterminated string;
- invalid escape;
- unpaired surrogate under chosen policy;
- leading zero number;
- missing comma/colon;
- trailing comma;
- invalid UTF-8 policy case;
- excessive nesting;
- trailing non-whitespace;
- file-size limit.

Every malformed test checks diagnostic position and confirms no partial trusted model is returned.

## 5. Dependency-file cases

| ID | Input feature | Expected |
|---|---|---|
| DEP-01 | simple target/prereqs | parsed rule |
| DEP-02 | backslash-newline | logical line joined |
| DEP-03 | escaped space | one path token |
| DEP-04 | escaped `#` | literal hash |
| DEP-05 | escaped backslash | correct token |
| DEP-06 | Windows-like colon if unsupported | clear fallback, not misparse |
| DEP-07 | multiple targets | policy enforced |
| DEP-08 | duplicate prerequisites | deduplicated deterministically |
| DEP-09 | missing colon | malformed |
| DEP-10 | trailing escape | malformed |
| DEP-11 | empty prerequisites | parsed only if compiler output permits; policy documented |
| DEP-12 | CRLF | accepted |

## 6. Path cases

- project-relative path;
- absolute inside project;
- absolute inside build;
- `.` and repeated separators;
- `..` remaining inside root;
- `..` escaping root;
- deleted/nonexistent path;
- duplicate lexical equivalents;
- case-different paths on Linux;
- path with spaces/hash;
- NUL rejection;
- newline format limitation;
- symlink policy case.

## 7. CMake loader cases

- one valid reply index and codemodel;
- missing index;
- multiple index files with/without explicit choice;
- missing referenced codemodel;
- unsupported codemodel major;
- one configuration;
- multiple configurations without option;
- explicit valid/invalid configuration;
- target JSON missing;
- target id duplicate/mismatch;
- executable with one artifact;
- target with generated source;
- dependency references unknown target;
- referenced JSON attempts path traversal.

## 8. CTest cases

- valid empty test array;
- valid multiple direct executable tests;
- duplicate test name;
- missing/invalid `kind`;
- unsupported major;
- missing command;
- direct absolute artifact match;
- unambiguous relative artifact match;
- basename-only false match rejected;
- wrapper/interpreter command rejected for subset;
- zero artifact matches;
- multiple artifact matches;
- properties ignored safely.

## 9. Impact cases

1. changed test source → its target → its test;
2. changed implementation source → dependent test target(s);
3. changed private header → one translation unit/test;
4. changed shared header → multiple tests;
5. changed source used by multiple targets;
6. target dependency chain depth > 1;
7. diamond dependency graph;
8. graph cycle terminates and remains deterministic;
9. two changed files converge on one test without duplication;
10. unaffected known file produces safely empty subset only if allowed;
11. explanation takes shortest deterministic path;
12. output order independent of input order.

## 10. Safety mutation matrix

Begin with one fixture that successfully selects a subset. For each mutation below, assert it no longer returns subset:

- delete File API index;
- corrupt index JSON;
- delete one target JSON;
- remove one target source;
- change selected configuration;
- delete one relevant `.d`;
- corrupt one relevant `.d`;
- make `.d` stale under supported check;
- add unknown changed path;
- change `CMakeLists.txt`;
- change generated source;
- delete CTest JSON;
- corrupt CTest JSON;
- wrap one test executable in shell/Python;
- duplicate an artifact mapping;
- insert unsupported format major;
- move metadata under mismatched build root.

## 11. Integration fixture scenarios

### Scenario A: narrow change

Change `include/parser/token.hpp`, used only by parser test path. Expect `ParserUnitTests` only.

### Scenario B: shared change

Change `include/common/error.hpp`, used by parser and lexer paths. Expect both mapped tests.

### Scenario C: unrelated mapped source

Change a tool source with no test-dependent reverse edge. Expect empty subset only after global completeness is proven; otherwise all.

### Scenario D: missing evidence

Remove parser `.d`. Expect all known tests and fallback status.

### Scenario E: missing catalogue

Remove CTest JSON. Expect `FULL_SUITE_REQUIRED`, no names.

### Scenario F: wrapper

Replace direct command in synthetic CTest metadata with a wrapper. Expect all known tests.

## 12. Performance and robustness

- generate synthetic graphs with 10k, 100k, and if feasible 1M edges;
- verify traversal linearity within noise;
- test maximum nesting and file size boundaries;
- run malformed-input corpus repeatedly;
- run under sanitizers if available as dev tooling and disclose that they are not runtime dependencies;
- verify 4 GB machine remains responsive.

## 13. Determinism

- shuffle changed-path order;
- shuffle filesystem discovery order through fixture naming;
- repeat analysis 20 times;
- compare stdout and stderr bytes;
- compare explanation tie-break outcomes.

## 14. Dependency/prohibition verification

- search implementation for process-spawn APIs and shell commands;
- inspect build logs for external downloads/lookups;
- inspect dynamic dependencies (`ldd` on Linux) and explain system libraries;
- verify no vendored source or generated library code;
- build in a clean directory with networking disabled if practical.

## 15. Release blockers

- any mutation case returns `SUBSET_SELECTED`;
- catalogue failure emits invented/partial names;
- output order varies;
- external process execution exists;
- parser crash, hang, or unbounded recursion on bounded malformed input;
- docs claim support not covered by tests;
- clean build or demo fails.

## 16. Minimum completion numbers

Aim for at least:

- 10 CLI/output tests;
- 15 JSON tests;
- 12 dependency parser tests;
- 10 path tests;
- 10 metadata mapping tests;
- 12 impact/safety tests;
- 5 end-to-end scenarios.

Coverage quantity is secondary to mutation quality. A small test suite that proves conservative behavior is stronger than many trivial assertions.

