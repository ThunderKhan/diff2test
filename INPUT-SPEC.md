# Input Specification — TestImpact++ MVP

## 1. General rules

- Inputs are treated as untrusted data.
- Text input is expected to be UTF-8; ASCII is a valid subset.
- Embedded NUL is rejected.
- File-size and nesting limits must be documented in `--help` or README.
- Unsupported major format versions are never parsed optimistically.
- A malformed file is identified by path and position where possible.
- Paths are compared only after normalization under the declared roots.

## 2. Changed-files input

### Source

Exactly one of:

- `--changed-files <path>`
- `--changed-files -` for stdin

### Grammar

- one path per line;
- LF and CRLF accepted;
- final line may omit newline;
- blank lines ignored;
- leading/trailing whitespace is part of a path, not automatically trimmed, except a trailing CR in CRLF;
- duplicate normalized paths are removed;
- NUL and newline inside a path are unsupported.

### Path form

- preferred form: relative to `--project-root` using `/` separators;
- absolute paths are accepted only if normalization proves they are within the project or build root;
- `.` segments are removed;
- `..` is resolved lexically and rejected if it escapes the allowed root;
- symlink identity is not resolved in MVP unless `weakly_canonical` is safely available and its failure behavior is explicit;
- case comparison follows the host filesystem; Linux MVP is case-sensitive.

### Git rename/copy output

MVP accepts only one path per line, as produced by ordinary `git diff --name-only`. It does not parse `--name-status`, quoted Git path syntax, `-z`, or `old => new` display abbreviations. Users must supply the actual affected paths, typically both old and new names when deletion/rename semantics matter.

### Empty list

An empty list is an invocation error by default because it often indicates a broken pipeline. A future `--allow-empty` option may explicitly permit a no-tests result.

## 3. CMake File API reply

### Location

`--cmake-reply <directory>` points to a CMake File API v1 reply directory, normally:

```text
<build>/.cmake/api/v1/reply
```

### Index selection

The directory may contain `index-*.json`. The MVP must define a deterministic policy:

1. if exactly one supported index exists, use it;
2. if several exist, choose the lexicographically greatest filename only if documented as the requested policy and warn that filenames are unspecified; safer MVP alternative: require `--cmake-index <path>`;
3. if selection is ambiguous or the referenced files are missing, fall back.

Recommended kickoff decision: add optional `--cmake-index` and require it when multiple indexes exist.

### Required index data

- valid JSON object;
- CMake reply metadata sufficient to locate a `codemodel` object with supported major version 2;
- referenced codemodel file must remain inside the reply directory after normalization.

### Required codemodel data

- `kind`/version where present and applicable;
- `paths.source` and `paths.build`;
- `configurations` array;
- selected configuration name;
- target entries with `id`, `name`, and `jsonFile`.

### Required target-object data

For each in-scope target:

- `name`
- `id`
- `type`
- `paths` as needed
- `sources[].path`
- `sources[].isGenerated` when present
- `artifacts[].path` for executables
- `dependencies[].id` for supported propagation
- compile/source association fields only if required to connect `.d` evidence

### Configuration

- if exactly one configuration exists, it may be selected implicitly;
- if more than one exists, `--configuration <name>` is required;
- zero or multiple same-name matches trigger fallback;
- cross-configuration target/artifact mixing is prohibited.

### Generated sources

If an affected path maps to `isGenerated: true` or requires an unmodeled custom-command relationship, MVP falls back to the full suite.

## 4. Compiler dependency files

### Source

One of the post-spike supported strategies:

- `--dep-root <directory>` recursively finds regular files with `.d` suffix; or
- `--dep-list <file>` gives one dependency-file path per line.

If both are supported, they are mutually exclusive.

### Supported syntax

GCC/Clang Make-style dependency rules:

```text
target: prerequisite prerequisite ...
```

MVP parser requirements:

- backslash-newline continuation;
- spaces/tabs as separators outside escapes;
- escaped space, tab, `#`, `:`, and backslash as observed in supported compiler output;
- one or more targets before `:` if encountered, though only a uniquely mapped object target is trusted;
- repeated rules may be combined only under an explicit deterministic policy;
- CRLF accepted;
- malformed trailing escape rejected.

### Mapping requirements

The rule target must map uniquely to a compiled object/translation unit relationship derived from the supported build layout or metadata. If it cannot, the `.d` file cannot silently contribute partial evidence.

### Completeness

For every in-scope compiled translation unit, the analyzer needs a supported dependency file or an explicit rule proving that the source has no separate compilation unit. Missing relevant `.d` evidence triggers fallback.

### Staleness

If the dependency file is older than a source/prerequisite under a chosen timestamp policy, mark it stale and fall back. Document that timestamp checks cannot prove perfect freshness.

## 5. CTest JSON

### Source

`--ctest-info <path>` identifies a file previously produced outside TestImpact++ by:

```bash
ctest --show-only=json-v1 > build/ctest-info.json
```

This command is documentation for the user's workflow; TestImpact++ never runs it.

### Required top-level data

- object;
- `kind` equals `ctestInfo`;
- `version.major` equals `1`;
- `tests` is an array.

### Required per-test data

- non-empty unique `name` string;
- non-empty `command` array whose first element is a string executable token;
- properties retained only when needed for safety checks.

### Mapping normalization

The command's first token is resolved relative to documented CTest/build context only when that context is unambiguous. It must match exactly one executable artifact after normalization. Basename-only matches are insufficient.

### Unsafe commands

Examples that trigger full-suite fallback in MVP:

- `python test.py ...`
- `/bin/sh wrapper.sh ...`
- emulator/launcher followed by target path
- command token absent or not a string
- unresolvable relative executable
- exact path matching zero or multiple artifacts

## 6. Project and build roots

- `--project-root` must exist and be a directory.
- `--build-root` should exist and normally match codemodel `paths.build` after normalization.
- A mismatch triggers fallback unless a documented relocation mode safely reconciles them; relocation is not MVP.
- Inputs referenced by metadata must remain within reply/build/project roots except system headers, which may be recorded but do not become changed project nodes.

## 7. Input limits

Initial recommended limits, adjustable after measurement:

- JSON file: 64 MiB each
- JSON nesting: 256
- JSON string: 16 MiB
- dependency file: 16 MiB each
- changed paths: 100,000 lines
- total graph nodes: 2,000,000
- explanation depth: bounded by visited-node count

Exceeding a limit is a safe analysis failure, not a crash.

## 8. Version policy

- Support known major versions only.
- Accept newer minor versions when required fields retain compatible types and unknown members are ignored.
- Reject/fallback on newer major versions.
- Print detected versions in verbose diagnostics.

## 9. Malformed and absent input matrix

| Input | Missing | Malformed/unsupported | Safety result |
|---|---|---|---|
| changed paths | usage failure | usage failure or full suite for unsafe path | no subset |
| CMake reply | full known suite if CTest readable | full known suite | no subset |
| `.d` evidence | full known suite | full known suite | no subset |
| CTest JSON | `FULL_SUITE_REQUIRED` | `FULL_SUITE_REQUIRED` | external full run required |
| roots/configuration | usage failure or full suite depending on detectability | full suite | no subset |

## 10. Future inputs not in MVP

- `compile_commands.json`
- Ninja `.ninja_deps`
- MSVC `/sourceDependencies`
- raw Git patch or `-z` format
- coverage database
- manual target/test manifest
- CMake trace output
- test history

