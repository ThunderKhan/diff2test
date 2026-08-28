# Input Specification — diff2test MVP

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
- symlink identity is not blindly canonicalized in MVP;
- case comparison follows the host filesystem; Linux MVP is case-sensitive.

### Git rename/copy output

MVP accepts only one path per line, as produced by ordinary `git diff --name-only`. It does not parse `--name-status`, quoted Git path syntax, `-z`, or `old => new` display abbreviations. Users must supply the actual affected paths, typically both old and new names when deletion/rename semantics matter.

### Empty list

An empty list is an invocation error by default because it often indicates a broken pipeline.

## 3. CMake File API reply

### Location

`--cmake-reply <directory>` points to a CMake File API v1 reply directory, normally:

```text
<build>/.cmake/api/v1/reply
```

The File API query is created externally under the build tree before CMake configure. diff2test never invokes CMake.

### Index selection

- if exactly one `index-*.json` exists, it may be used;
- when reply selection is ambiguous, `--cmake-index <path>` may identify the intended index;
- missing, ambiguous, malformed, unsupported, or escaping references trigger safe full-suite behavior.

### Required codemodel data

- supported codemodel major version 2;
- `paths.source` and `paths.build` matching declared roots;
- one selected or unambiguous configuration;
- target entries with ids and referenced target JSON;
- target names/types/sources/artifacts/dependencies needed by the MVP.

### Configuration

- one configuration may be selected implicitly;
- if more than one exists, `--configuration <name>` is required;
- zero or duplicate matches trigger fallback;
- cross-configuration target/artifact mixing is prohibited.

### Generated sources

Generated sources/custom-command relationships are not modeled by the current MVP. Encountering a generated source in the supported model triggers full-suite fallback.

## 4. Compiler dependency files

### Source — frozen MVP decision

The first metadata spike found that a CMake build tree can contain `.d` files that are not compiler prerequisite evidence (for example `link.d`). Therefore the MVP deliberately does **not** recursively treat every `.d` file as a compiler dependency file.

The supported input is:

```text
--dep-list <file>
```

The list contains one dependency-file path per line. Relative entries are resolved under `--build-root`. The files themselves must already have been produced by the external compiler/build workflow; diff2test never invokes that workflow.

### Supported syntax

GCC/Clang Make-style dependency rules:

```text
target: prerequisite prerequisite ...
```

The parser supports the tested subset needed for generated dependency files, including backslash-newline continuation, escaped tokens, CRLF, and deterministic prerequisite handling. Malformed input is never treated as an empty dependency relationship.

### Current target-mapping boundary

For the controlled MVP using CMake's Unix Makefiles-style object layout, the dependency rule target must map uniquely to a CMake target through the observed layout:

```text
CMakeFiles/<target-name>.dir/...
```

This is a deliberately narrow, evidence-backed boundary—not a claim that all CMake generators use this layout. An unmatched or ambiguous layout triggers full-suite fallback.

### Translation-unit completeness

Coverage is tracked per **(CMake target, compiled source)** pair. This matters when one source file is compiled into more than one target: evidence for one target does not prove evidence for the others.

Before `SUBSET_SELECTED` is allowed, every supported compiled `.cpp` translation unit in the in-scope codemodel must have exactly one trusted dependency-file mapping. Missing or duplicate evidence triggers `FULL_SUITE_SELECTED` when the test catalogue is known.

### Freshness

The current implementation does not yet claim that supplied metadata is fresh merely because it exists. Detectable stale-evidence handling remains a release-hardening requirement; final documentation must not claim absolute freshness or soundness for stale/adversarial metadata.

## 5. CTest JSON

### Source

`--ctest-info <path>` identifies a file previously produced outside diff2test by:

```bash
ctest --show-only=json-v1 > build/ctest-info.json
```

This command belongs to the user's external workflow; diff2test never runs it.

### Required data

- top-level `kind` equals `ctestInfo`;
- `version.major` equals `1`;
- `tests` is an array;
- each test has a non-empty unique name and non-empty string command array.

### Mapping normalization

The first command token must resolve to exactly one CMake executable artifact after build-root resolution and lexical normalization. Basename-only matches are prohibited.

Wrapper/interpreter commands, zero matches, and ambiguous matches cause full-known-suite fallback in the MVP.

## 6. Project and build roots

- `--project-root` and `--build-root` must exist and be directories;
- normalized declared roots must match codemodel source/build roots;
- referenced metadata files must remain within their declared containers;
- dependency prerequisites may include external/system headers, but changed-path analysis concerns project paths;
- path traversal/root escape is rejected.

## 7. Initial input limits

- JSON file: 64 MiB each
- JSON nesting: 256
- JSON string: 16 MiB
- dependency file: 16 MiB each
- dependency-list file: 4 MiB
- changed paths: bounded by available memory for the MVP; final README will document the tested practical boundary

Exceeding a configured limit is a safe analysis failure, not a crash.

## 8. Version policy

- support known major versions only;
- accept compatible unknown minor fields only when required structure remains valid;
- reject/fallback on unsupported major versions;
- never silently reinterpret an unknown major format.

## 9. Malformed and absent input matrix

| Input | Missing/malformed | Safety result |
|---|---|---|
| changed paths | invalid/empty invocation | no subset / usage error |
| CMake reply/codemodel | catalogue known | `FULL_SUITE_SELECTED` |
| required `.d` evidence | catalogue known | `FULL_SUITE_SELECTED` |
| unknown changed project path | catalogue known | `FULL_SUITE_SELECTED` |
| CTest JSON | unavailable/untrusted | `FULL_SUITE_REQUIRED` |
| root/configuration ambiguity | catalogue known | `FULL_SUITE_SELECTED` |

## 10. Future inputs not in MVP

- recursive `--dep-root` discovery without a proven generator-specific policy
- `compile_commands.json`
- Ninja `.ninja_deps`
- MSVC `/sourceDependencies`
- raw Git patch or `-z` format
- coverage database
- manual target/test manifest
- CMake trace output
- test history
