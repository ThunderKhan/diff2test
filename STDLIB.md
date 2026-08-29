# STDLIB.md — diff2test

This file documents the zero-dependency design of `diff2test`: which capabilities are often delegated to third-party packages, what this project uses instead, and where externally generated metadata/tooling sits relative to the runtime artifact.

## Rule boundary

`diff2test` ships with **zero third-party runtime code**. Its implementation is one C++20 source file, `diff2test.cpp`, using the C++ standard library and permitted system/toolchain runtime facilities.

CMake, CTest, Git, compilers, sanitizers, `ldd`, and shell commands shown in CI or documentation are **external build/development/proof tooling**. They are not invoked by the `diff2test` process.

The organizer clarified two points that are central to this design:

1. the compiler/build tool does not count as a runtime dependency;
2. parsing files previously produced by tools such as CMake/CTest is permitted when disclosed and when absence degrades gracefully.

Therefore the precise statement is:

> **CMake is permitted external build/input-generation tooling, not a third-party runtime dependency of `diff2test`.**

`diff2test` does not contain a CMake library, does not launch CMake, and does not require a CMake process when analyzing already-generated metadata.

## Real substitutions

| Capability commonly delegated to a package | Typical package/category | `diff2test` replacement | Why this was reasonable here |
|---|---|---|---|
| Command-line parsing | CLI11 / cxxopts | direct `argc`/`argv` parsing with `std::string`, `std::map`, `std::set`, `std::optional` | the CLI is intentionally small and frozen; explicit validation is easier to audit |
| JSON parsing | nlohmann/json / RapidJSON | purpose-built recursive-descent JSON parser | required strict format/version handling without introducing a parser dependency |
| JSON source diagnostics | parser-library diagnostics | custom byte/line/column cursor and error records | malformed metadata must fail visibly and deterministically |
| JSON value model | JSON DOM package | `std::variant`, `std::vector`, `std::map`, `std::string` | enough for File API and CTest JSON without a runtime library |
| UTF-8/Unicode escape handling | Unicode/JSON helper library | custom UTF-8 validation plus `\uXXXX`/surrogate-pair decoding | keeps JSON acceptance rules explicit and tested |
| Filesystem/path handling | Boost.Filesystem/platform helper | `std::filesystem::path` plus lexical containment checks | C++20 provides the required path primitives; custom policy prevents root escapes |
| Make dependency parsing | Makefile/parser package | purpose-built state/scanning logic | only the observed GCC/Clang `.d` grammar subset is needed |
| Dependency deduplication | utility/range helpers | ordered `std::set`/`std::vector` handling | deterministic evidence without another utility package |
| Graph representation | Boost.Graph | `std::map`, `std::set`, `std::vector` adjacency/index structures | the impact graph is small conceptually and benefits from visible edge semantics |
| Graph traversal | Boost.Graph algorithms | `std::queue` + visited/predecessor maps | straightforward reverse dependency traversal with deterministic explanation parents |
| Result/error handling | `expected`/result library | `std::optional` + project result/error structs | explicit failure paths without adding a result framework |
| Output formatting | fmt | `std::ostream`, `std::string`, ordered containers | output surface is small and byte-stability is tested directly |
| Logging/diagnostics | spdlog | stderr/stdout separation and structured project diagnostics | no asynchronous/global logging system is needed |
| String tokenization | string utility libraries | `std::string_view` plus format-aware scanners | generic splitting is incorrect for escaped Make-style tokens and JSON |
| Sorting/determinism | ranges/helper libraries | `<algorithm>` and ordered standard containers | user-visible ordering is intentionally stable and tested |
| Test framework | Catch2 / GoogleTest | small C++ test executables using direct checks | repository tests stay dependency-free too; no test framework ships at runtime |
| Sanitizer tooling | packaged runtime framework | compiler-provided ASan/UBSan in a separate CI build | dev-only verification; sanitizer linkage is not part of the normal shipped artifact |

These are genuine substitutions present in the current implementation/test workflow; this table intentionally omits features that were never implemented.

## Package Killer target: RTS++ / Ekstazi++

For the Package Killer bonus, the primary comparison target is [`EngineeringSoftware/ekstazipp`](https://github.com/EngineeringSoftware/ekstazipp), which describes itself as a **Regression Test Selection tool for C++**.

Its CMake build requires LLVM via `find_package(LLVM REQUIRED CONFIG)`, builds an LLVM pass and `ekstazi-lib`, and installs CMake package/export files. Its README also points to separate SHA-512 source as a dependency. In contrast, `diff2test` implements the narrow supported CMake/CTest RTS workflow with one C++20 runtime source and no third-party runtime package.

The claim is deliberately scoped: `diff2test` is **not** a drop-in replacement for all RTS++ functionality. It replaces the dedicated RTS stack only for the tested workflow where CMake File API, compiler `.d`, CTest JSON, and changed-path evidence are already available.

See [`PACKAGE-KILLER.md`](PACKAGE-KILLER.md) for the complete side-by-side comparison, dependency evidence, substitutions, limitations, and CI proof.

## What the standard library handled well

### `std::filesystem`

It provides portable path objects, lexical normalization primitives, directory iteration, file metadata, and timestamps. The project still adds its own containment policy because a filesystem type alone does not decide whether a path is safe to trust as project/build evidence.

### Containers and algorithms

`std::map`, `std::set`, `std::vector`, `std::queue`, and `<algorithm>` are sufficient for the target/test/dependency graph at this scope. Ordered containers also make deterministic output easier to defend.

### `std::variant` and `std::optional`

They make a compact JSON value tree and explicit optional/error states possible without a general-purpose JSON/result package.

### Streams and strings

`std::ifstream`, `std::ostream`, `std::string`, and `std::string_view` cover input/output and parser storage needs. Specialized syntax still requires custom scanners rather than naive whitespace splitting.

## What had to be written from first principles

The standard library supplies primitives, not domain semantics. The project implements its own:

- strict JSON grammar and diagnostics;
- UTF-8 and JSON Unicode-escape validation;
- Make-style `.d` parsing;
- CMake File API and CTest schema validation for the supported major versions;
- exact executable-artifact mapping;
- translation-unit completeness checks;
- target-impact graph construction/traversal;
- evidence/predecessor capture for explanations;
- central conservative safety/fallback policy;
- CLI contract and deterministic formatting.

## External data is not a runtime library dependency

`diff2test` consumes data that may have been produced by other tools:

| Input | Typical external producer | Runtime behavior if unavailable/untrusted |
|---|---|---|
| changed-path list | user, CI, or external Git pipeline | invalid/empty input is rejected; unknown project paths cause conservative fallback |
| CMake File API reply | external CMake configure step | full known suite if CTest catalogue remains trustworthy |
| GCC/Clang `.d` files | external compiler/build step | full known suite if evidence is missing/malformed/stale/ambiguous |
| CTest JSON | external `ctest --show-only=json-v1` | `FULL_SUITE_REQUIRED`; no test names invented |

These are parsed as untrusted files. The runtime does not execute their producers.

## CMake specifically

CMake appears in this repository in two permitted roles:

1. **Build tool for `diff2test` itself.** A judge may use CMake to compile the single C++ source file.
2. **External metadata producer for a CMake project being analyzed.** The File API query/reply is generated before `diff2test` runs.

Neither role makes CMake a linked/runtime package of the executable. The normal runtime source contains no process-spawning path that could invoke CMake, CTest, Git, a compiler, Python, or a shell.

If already-generated inputs are present, running `diff2test analyze ...` does not need a CMake process. If those inputs are not present, the program degrades conservatively rather than generating them itself.

## Build and verification tooling

Current public CI uses runner-provided tools for development verification:

- Git — fetches the exact repository commit for CI;
- CMake — configures/builds the project and controlled fixture;
- GCC — compiles C++20 code and produces `.o.d` fixture evidence;
- CTest — executes repository test binaries and exports fixture `json-v1` metadata;
- Bash/coreutils — CI orchestration and byte comparisons;
- `ldd` — dynamic-link proof;
- GCC AddressSanitizer/UndefinedBehaviorSanitizer — dev-only hardened test build.

None of these tools is launched from `diff2test.cpp`.

The normal Release artifact is built separately from the sanitizer build.

## Dependency proof

See [`DEPENDENCY-PROOF.md`](DEPENDENCY-PROOF.md). The public CI additionally searches `diff2test.cpp` for common process-spawn APIs and inspects Linux dynamic linkage.

## Honest tradeoffs

Avoiding mature packages has costs:

- the JSON parser must be carefully tested instead of inheriting years of library hardening;
- the CLI is intentionally small rather than feature-rich;
- Make `.d` support is tied to a tested grammar/generator boundary;
- the graph implementation favors clarity over specialized performance machinery;
- Linux/GCC-or-Clang/Unix-Makefiles scope is much narrower than a production multi-platform tool;
- timestamp freshness checks can detect some stale metadata but cannot cryptographically prove source/metadata correspondence;
- wrappers, interpreters, generated/custom-command chains, and unknown formats intentionally fall back instead of being guessed through.

Those limits are part of the safety design, not hidden compatibility claims.

## Submission checklist

- [x] 10+ genuine substitutions documented
- [x] externally generated metadata disclosed
- [x] CMake explicitly classified as permitted build/input-generation tooling rather than runtime dependency
- [x] development/test tooling distinguished from runtime
- [x] no vendored third-party source claimed or used
- [x] Package Killer target and feature/dependency comparison documented in `PACKAGE-KILLER.md`
- [x] Package Killer scope is explicitly narrower than a drop-in RTS++ replacement
- [x] dependency-proof document linked
