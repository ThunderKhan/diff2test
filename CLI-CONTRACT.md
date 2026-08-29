# CLI Contract — diff2test MVP

## 1. Executable

Canonical artifact name:

```text
diff2test
```

Product name in prose: **diff2test**.

## 2. Commands

```text
diff2test analyze [project-root] [options]
diff2test --help
diff2test --version
```

Only `analyze` performs analysis. Unknown commands and options are usage errors.

## 3. Analyze synopsis

```bash
diff2test analyze [project-root] \
  [--build <dir> | --build-root <dir>] \
  [--changed-files <file|->] \
  [--cmake-reply <dir>] \
  [--ctest-info <file>] \
  [--dep-list <file>] \
  [--cmake-index <file>] \
  [--configuration <name>] \
  [--format human|names] \
  [--explain] \
  [--verbose]
```

Convention defaults:

```text
project-root    .
build-root      <project-root>/build
changed-files   stdin
cmake-reply     <build-root>/.cmake/api/v1/reply
ctest-info      <build-root>/ctest-info.json
dep-list        <build-root>/deps.txt
```

The metadata spike found non-compilation `.d` files such as `link.d` in build trees. To avoid generator-specific guessing, the MVP deliberately does **not** recursively discover `.d` files. The dependency-list path merely has a deterministic default; the existing completeness and target/TU validation remains authoritative.

## 4. Options

| Option | Required | Implemented MVP meaning |
|---|---:|---|
| positional `[project-root]` | no | top-level source root; defaults to `.` |
| `--project-root <dir>` | no | explicit alternative to positional project root; using both is a usage error |
| `--build <dir>` | no | convenience alias for build root |
| `--build-root <dir>` | no | build-tree root; defaults to `<project-root>/build`; cannot be combined with `--build` |
| `--changed-files <file|->` | no | newline-delimited changed paths; omitted or `-` reads stdin |
| `--cmake-reply <dir>` | no | File API v1 reply directory; defaults to `<build-root>/.cmake/api/v1/reply` |
| `--cmake-index <file>` | no | explicit index when reply-directory selection would otherwise be ambiguous |
| `--ctest-info <file>` | no | pre-generated CTest JSON; defaults to `<build-root>/ctest-info.json` |
| `--dep-list <file>` | no | newline-delimited explicit list of compiler dependency files; defaults to `<build-root>/deps.txt` |
| `--configuration <name>` | conditional | selects one codemodel configuration when more than one exists |
| `--format <value>` | no | `human` (default) or `names` |
| `--explain` | no | include evidence chains in human output |
| `--verbose` | no | accepted by the frozen CLI; no additional output is currently emitted by the MVP |
| `-h`, `--help` | no | print concise usage/options/tool-boundary help and exit 0 |
| `--version` | no | print version and exit 0 |

The MVP does not implement color or `--no-color`.

## 5. Runtime prohibition

No CLI option generates metadata, runs tests, invokes Git, or launches another program. The runtime contains no supported `--run`, `--generate`, `--git-base`, or similar option.

CMake, CTest, Git, the compiler, and shell utilities may generate inputs externally. `diff2test` only reads files and stdin. A shell pipeline such as:

```bash
git diff --name-only HEAD~1 | diff2test analyze .
```

is caller-side composition: the shell launches Git and writes newline-delimited paths to `diff2test` stdin.

## 6. Output contract

### 6.1 Human subset success

Human mode begins with the symbolic outcome and lists selected tests. With `--explain`, it also emits the concrete evidence chain.

Representative verified shape:

```text
STATUS: SUBSET_SELECTED

Selected tests (1):
  AlphaTest

Reason for AlphaTest:
  changed path: include/alpha.hpp
  dependency file: CMakeFiles/alpha.dir/src/alpha.cpp.o.d
  translation unit: src/alpha.cpp
  owning target: alpha
  dependent target: alpha_test
  registered test: AlphaTest
```

The exact dependency-file/target chain reflects the metadata for the analyzed project.

### 6.2 Names subset success

stdout contains names only:

```text
AlphaTest
```

A successful subset has no mandatory stderr summary. Diagnostics/fallback reasons are written to stderr when present.

### 6.3 Full-known-suite fallback

When the CTest catalogue is trustworthy but other evidence is unsafe:

Human stdout begins:

```text
STATUS: FULL_SUITE_SELECTED
```

Names mode emits every known test, one per line.

stderr uses the stable symbolic status prefix followed by the concrete reason, for example:

```text
diff2test: FULL_SUITE_SELECTED: missing dependency evidence for source src/alpha.cpp in target <target-id>
```

The process exits `10`.

### 6.4 Catalogue unavailable

Human mode prints:

```text
STATUS: FULL_SUITE_REQUIRED
```

stderr contains the reason with the same symbolic prefix, for example:

```text
diff2test: FULL_SUITE_REQUIRED: CTest catalogue not found at build/ctest-info.json
```

Names mode emits no invented test names. The process exits `11`.

## 7. stdout/stderr separation

### stdout

- selected/all-known test data;
- human report body;
- status marker in human mode;
- evidence chains requested with `--explain`.

### stderr

- invalid invocation diagnostics;
- analysis/fallback reasons;
- internal-error diagnostics.

Machine consumers using names format can capture stdout without diagnostic contamination.

## 8. Exit statuses

| Code | Symbolic meaning | Implemented meaning |
|---:|---|---|
| `0` | `SUBSET_SELECTED` | supported evidence audit completed and the emitted impacted-test set was selected |
| `10` | `FULL_SUITE_SELECTED` | CTest catalogue is known, but another evidence source is unsafe; all known tests are emitted |
| `11` | `FULL_SUITE_REQUIRED` | catalogue cannot be trusted/enumerated; caller must run its normal full suite |
| `64` | `USAGE_ERROR` | invalid command/options or empty changed-path input |
| `65` | `INPUT_ERROR` | reserved input-error outcome in the core status enum; current conservative analysis paths normally widen to 10/11 instead |
| `70` | `INTERNAL_ERROR` | unexpected exception/invariant boundary |

Important: shell truthiness treats 10 and 11 as failure-like values. Integrations must handle them as deliberate safety statuses, not as crashes. This makes fallback hard to overlook.

## 9. Deterministic ordering

Verified behavior:

- changed paths are sorted/deduplicated before impact classification;
- selected/fallback test names are lexical because the CTest catalogue is normalized;
- graph traversal uses ordered adjacency/visited structures;
- explanation predecessor choice is deterministic for the supported graph;
- reordered `.d` list input and duplicate changed paths produce byte-identical output in the real fixture;
- shorthand and fully explicit invocations produce the same analysis result;
- CI repeats the same real analysis 20 times and compares stdout/stderr byte-for-byte.

## 10. Help contract

The built-in `--help` is intentionally concise. It states:

- the available commands;
- the positional project root and deterministic defaults;
- explicit override flags;
- that omitted `--changed-files` reads stdin;
- that CMake, CTest, Git, and the compiler may generate input externally;
- that `diff2test` never launches them or any other program at runtime.

The complete platform/metadata boundaries, external input-generation examples, safety outcomes, and exit table live in `README.md` and this contract rather than duplicating a long manual inside the binary help string.

## 11. Invalid invocation and analysis boundaries

Verified usage-error (`64`) cases include:

- missing command;
- unknown command;
- unknown option, including removed `--dep-root`;
- repeated single-valued option;
- missing option value;
- invalid `--format` value;
- more than one positional project root;
- using both positional project root and `--project-root`;
- using both `--build` and `--build-root`;
- empty changed-path input after required metadata/catalogue inputs can be loaded.

Some invalid-looking analysis inputs are deliberately **safety fallbacks**, not usage errors. For example, a declared project/build root that does not match trusted codemodel evidence produces `FULL_SUITE_SELECTED` when the catalogue is known. Unsupported/invalid configuration selection likewise flows through the metadata safety path rather than being guessed through.

The MVP does not attempt TTY detection. Omitted `--changed-files` and explicit `--changed-files -` both consume stdin.

## 12. Compatibility policy

The legacy fully explicit form remains supported. The shorthand interface is a convention-over-configuration layer over the same analysis engine and safety outcomes; it does not weaken completeness checks or add hidden process execution.
