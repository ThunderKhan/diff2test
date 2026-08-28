# CLI Contract — diff2test MVP

## 1. Executable

Canonical artifact name:

```text
diff2test
```

Product name in prose: **diff2test**.

## 2. Commands

```text
diff2test analyze [options]
diff2test --help
diff2test --version
```

Only `analyze` is required for MVP. Unknown commands and options are usage errors.

## 3. Analyze synopsis

```bash
diff2test analyze \
  --project-root <dir> \
  --build-root <dir> \
  --changed-files <file|-> \
  --cmake-reply <dir> \
  --ctest-info <file> \
  (--dep-root <dir> | --dep-list <file>) \
  [--configuration <name>] \
  [--format human|names] \
  [--explain] \
  [--verbose]
```

Final choice between `--dep-root` and `--dep-list` is made by the first-two-hours spike. Supporting both is desirable but not mandatory.

## 4. Options

| Option | Required | Meaning |
|---|---:|---|
| `--project-root <dir>` | yes | top-level source root |
| `--build-root <dir>` | yes | build-tree root |
| `--changed-files <file|->` | yes | newline-delimited changed paths; `-` reads stdin |
| `--cmake-reply <dir>` | yes | File API v1 reply directory |
| `--cmake-index <file>` | no | explicit index when reply directory is ambiguous |
| `--ctest-info <file>` | yes | pre-generated CTest JSON |
| `--dep-root <dir>` | conditional | recursive dependency-file root |
| `--dep-list <file>` | conditional | explicit list of dependency files |
| `--configuration <name>` | conditional | required for multiple codemodel configurations |
| `--format <value>` | no | `human` default or `names` |
| `--explain` | no | show evidence paths in human format |
| `--verbose` | no | show detected versions/files and safety audit detail on stderr |
| `--no-color` | no/stretch | disable ANSI decoration if color is implemented |
| `-h`, `--help` | no | help and successful exit |

## 5. Runtime prohibition

No CLI option may generate metadata, run tests, invoke Git, or launch any program. Do not add options named `--run`, `--generate`, `--git-base`, or similar during MVP.

## 6. Output contract

### 6.1 Human subset success

```text
STATUS: SUBSET_SELECTED

Changed files:
  include/parser/token.hpp

Selected tests (1 of 3):
  ParserUnitTests

Reason:
  include/parser/token.hpp
    -> tests/parser_test.cpp
    -> parser_tests
    -> ParserUnitTests
```

### 6.2 Names subset success

stdout:

```text
ParserUnitTests
```

stderr, when verbose or explanation is requested:

```text
diff2test: SUBSET_SELECTED: selected 1 of 3 tests
```

### 6.3 Full-known-suite fallback

Human stdout lists all known tests and begins:

```text
STATUS: FULL_SUITE_SELECTED
```

stderr:

```text
diff2test: safety fallback: missing dependency evidence for target parser_tests
```

Names stdout still emits every known test, one per line, allowing the caller to run them.

### 6.4 Catalogue unavailable

stdout in human mode:

```text
STATUS: FULL_SUITE_REQUIRED
```

stderr:

```text
diff2test: cannot enumerate registered tests: CTest metadata is unavailable
diff2test: run the complete suite using your normal project workflow
```

Names mode emits no invented names.

## 7. stdout/stderr separation

### stdout

- selected test data;
- human report body;
- status marker in human mode.

### stderr

- invalid invocation diagnostics;
- parser/input diagnostics;
- safety fallback reasons;
- verbose metadata details;
- warnings.

Machine consumers using names format can capture stdout without diagnostic contamination.

## 8. Exit statuses

Recommended stable scheme:

| Code | Symbolic meaning | Description |
|---:|---|---|
| 0 | `SUBSET_SELECTED` | safe subset, including a provably empty subset if later allowed |
| 10 | `FULL_SUITE_SELECTED` | analysis uncertainty; all known tests emitted |
| 11 | `FULL_SUITE_REQUIRED` | suite cannot be enumerated; caller must run its normal full suite |
| 64 | `USAGE_ERROR` | invalid command/options/empty changed input |
| 65 | `INPUT_ERROR` | required input unreadable or structurally invalid where no safer enumerated result can be produced |
| 70 | `INTERNAL_ERROR` | invariant failure or unexpected exception |

Important: shell truthiness normally treats nonzero as failure. CI integration must explicitly handle 10 and 11 as safety statuses, not crash statuses. This deliberate choice makes fallback impossible to overlook. If integration ergonomics suffer, retain the distinction in a `STATUS:` record rather than silently converting fallback to success.

## 9. Deterministic ordering

- changed paths: normalized lexical order;
- selected tests: lexical name order;
- fallback reasons: stable category then path order;
- explanation paths: shortest evidence path, then lexical tie-break;
- no raw unordered-container iteration in output.

## 10. Help requirements

`--help` shall state:

- the tool analyzes but never runs tests;
- metadata must be generated externally;
- fallback status semantics;
- supported metadata/platform boundaries;
- example stdin pipeline;
- exit-status table or link to README;
- no external programs are invoked.

## 11. Invalid invocations

Usage error examples:

- missing required option;
- both `--dep-root` and `--dep-list`;
- `--changed-files -` while stdin is a terminal, if reliably detectable;
- unknown format/config option;
- repeated single-valued option;
- empty changed list without explicit future override;
- root argument that is not a directory.

## 12. Compatibility policy

CLI names and exit statuses freeze at MVP completion. Before that, changes must update `CLI-CONTRACT.md`, `SAFETY-CONTRACT.md`, tests, README examples, and demo commands together.
