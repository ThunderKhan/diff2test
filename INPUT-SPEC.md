# Input Specification — diff2test MVP

## 1. General rules

- Inputs are treated as untrusted data.
- Text input is expected to be UTF-8; ASCII is a valid subset.
- Embedded NUL is rejected.
- File-size and nesting limits are enforced where implemented and must be reflected in the final README.
- Unsupported major format versions are never parsed optimistically.
- Malformed structured inputs are never treated as empty evidence.
- Paths are compared only after lexical normalization under declared roots.

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
- a trailing CR from CRLF is removed;
- duplicate input lines are sorted/deduplicated before analysis;
- embedded NUL is rejected.

### Path form

- preferred form: relative to `--project-root` using `/` separators;
- absolute paths are accepted only if normalization proves they remain within the permitted project/build model;
- `.` and `..` are resolved lexically;
- root escape is rejected;
- symlink identity is not blindly canonicalized in MVP;
- Linux MVP path comparison is case-sensitive.

### Git boundary

A user's shell may produce this file or pipe newline-delimited paths from Git. diff2test does not parse raw patches, invoke Git, or support Git-specific rename/status syntaxes in the MVP.

Deleted or renamed paths that no longer appear in current evidence safely cause full-suite behavior when they cannot be mapped.

### Empty list

An empty changed-path list is a usage error because it commonly indicates a broken caller pipeline.

## 3. CMake File API reply

### Location

`--cmake-reply <directory>` points to a CMake File API v1 reply directory, normally:

```text
<build>/.cmake/api/v1/reply
```

The File API query is created externally under the build tree before CMake configure. CMake is permitted build/input-generation tooling under the organizer ruling; diff2test never invokes it at runtime.

### Index selection

- exactly one supported `index-*.json` may be selected implicitly;
- `--cmake-index <path>` may identify an intended index explicitly;
- missing, ambiguous, malformed, unsupported, or escaping references trigger full-known-suite behavior when the CTest catalogue is readable.

### Required codemodel data

The current MVP requires:

- codemodel major version 2;
- `paths.source` and `paths.build` matching the declared roots after normalization;
- one selected or unambiguous configuration;
- target references with ids and referenced target JSON;
- target names, types, sources, artifacts, and dependencies needed by the supported mapping.

### Configuration

- one configuration may be selected implicitly;
- if more than one configuration exists, `--configuration <name>` is required;
- zero or duplicate matches trigger fallback;
- cross-configuration target/artifact mixing is prohibited.

### Generated sources

Generated sources/custom-command chains are not modeled by the current MVP. Encountering a codemodel source marked generated prevents subset output.

## 4. Compiler dependency files

### Source — frozen MVP decision

The metadata spike found that CMake build trees can contain `.d` files that are not compiler prerequisite evidence, such as `link.d`. The MVP therefore deliberately does **not** scan every `.d` file recursively.

The supported input is:

```text
--dep-list <file>
```

The list contains one dependency-file path per line. Relative entries are resolved under `--build-root`. These dependency files must already have been produced by the external compiler/build workflow; diff2test never launches the compiler or build tool.

### Supported syntax

The parser handles the tested GCC/Clang Make-style dependency-rule subset:

```text
target: prerequisite prerequisite ...
```

Implemented handling includes:

- backslash-newline continuation;
- escaped tokens required by the tested format;
- CRLF;
- comments;
- deterministic prerequisite deduplication;
- malformed-rule rejection.

A missing or malformed rule never means “no dependencies.”

### Current target-mapping boundary

For the controlled Linux/CMake/Unix-Makefiles workflow, the compiler dependency rule target must map uniquely through the observed layout:

```text
CMakeFiles/<target-name>.dir/...
```

This is a deliberately narrow, tested MVP boundary. It is not a claim about Ninja, MSVC, Xcode, or every CMake generator. Unmatched or ambiguous target layouts cause `FULL_SUITE_SELECTED` when the test catalogue is known.

### Translation-unit completeness

Coverage is tracked per **(CMake target, compiled source)** pair. This matters when one source file is compiled into multiple targets: evidence for one target does not establish evidence for another.

For the supported `.cpp` translation units in the loaded codemodel, exactly one trusted dependency mapping is required before subset output. Missing evidence or duplicate translation-unit evidence widens to the full known suite.

### Implemented freshness audit

For each listed `.d` file, diff2test:

1. reads the dependency file's modification timestamp;
2. normalizes every prerequisite;
3. for prerequisites inside `--project-root`, reads the prerequisite modification timestamp;
4. rejects the dependency evidence if a project prerequisite is newer than the `.d` file;
5. also rejects subset output when a required project-prerequisite timestamp cannot be inspected.

Such failures become `FULL_SUITE_SELECTED` when the CTest catalogue is readable.

This is a **detectable-staleness check**, not proof of absolute freshness. It does not cryptographically bind metadata to source contents and does not claim that equal/older timestamps guarantee perfect correspondence. Users should generate the metadata as part of the same normal build workflow immediately before diff2test analysis.

External/system prerequisites may appear in `.d` data and are accepted as evidence input, but the current timestamp audit is intentionally applied to project-root prerequisites used by the project impact model.

## 5. CTest JSON

### Source

`--ctest-info <path>` identifies a file previously produced outside diff2test by:

```bash
ctest --show-only=json-v1 > build/ctest-info.json
```

CTest is external input-generation tooling; diff2test never invokes it or executes tests.

### Required data

- top-level `kind` equals `ctestInfo`;
- `version.major` equals `1`;
- `tests` is an array;
- each test has a non-empty unique name;
- each test has a non-empty string command array.

If the catalogue itself cannot be trusted or enumerated, the outcome is `FULL_SUITE_REQUIRED` and no test names are invented.

### Mapping normalization

The first CTest command token must resolve to exactly one CMake executable artifact after build-root resolution and lexical normalization. Basename-only matching is forbidden.

Wrapper/interpreter commands, zero matches, and ambiguous artifact matches prevent subset output and cause full-known-suite fallback.

## 6. Project and build roots

- `--project-root` and `--build-root` must exist and be directories;
- normalized declared roots must match the codemodel source/build roots;
- referenced File API metadata must stay within its declared reply container;
- dependency prerequisites may include external/system paths as evidence;
- changed-path analysis requires supported project paths;
- unsafe path traversal/root escape is rejected.

## 7. Input limits

Current configured limits include:

- JSON file: 64 MiB
- JSON nesting: 256
- JSON string: 16 MiB
- dependency file: 16 MiB
- dependency-list file: 4 MiB

Exceeding a configured limit is a safe analysis failure rather than a crash or partial trusted parse.

## 8. Version policy

- support known major versions only;
- tolerate unknown compatible minor/extra fields only when required structure remains valid;
- reject/fallback on unsupported major versions;
- never silently reinterpret a different major format.

## 9. Malformed and absent input matrix

| Input/condition | Safety result |
|---|---|
| changed paths missing/unreadable/empty | usage error / no subset |
| CTest catalogue missing or untrusted | `FULL_SUITE_REQUIRED` |
| CMake reply/codemodel missing, malformed, ambiguous, or root-mismatched | `FULL_SUITE_SELECTED` when catalogue known |
| dependency list or required `.d` missing/malformed | `FULL_SUITE_SELECTED` when catalogue known |
| dependency target or translation-unit mapping ambiguous | `FULL_SUITE_SELECTED` |
| duplicate dependency evidence | `FULL_SUITE_SELECTED` |
| project prerequisite newer than `.d` | `FULL_SUITE_SELECTED` |
| project prerequisite timestamp cannot be inspected | `FULL_SUITE_SELECTED` |
| unknown changed project path | `FULL_SUITE_SELECTED` |
| changed `CMakeLists.txt` / `.cmake` | `FULL_SUITE_SELECTED` |
| wrapper/unmapped CTest command | `FULL_SUITE_SELECTED` |

## 10. Verified controlled environment

The public CI exercises the real controlled fixture on Linux with CMake and the runner's GCC-compatible toolchain. It externally generates:

- File API codemodel replies;
- compiler `.o.d` files;
- CTest `json-v1` catalogue.

CI then verifies that changing `include/alpha.hpp` selects exactly `AlphaTest`, and that removing required dependency evidence widens to `AlphaTest`, `BetaTest`, and `CoreTest` with exit status 10.

That test validates the documented controlled environment only; it does not broaden the generator/platform support claim.

## 11. Future inputs not in MVP

- recursive `--dep-root` discovery without a proven generator-specific policy
- `compile_commands.json`
- Ninja `.ninja_deps`
- MSVC `/sourceDependencies`
- raw Git patch or `-z` format
- coverage database
- manual target/test manifest
- CMake trace output
- test history
