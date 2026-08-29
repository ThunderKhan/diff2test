# Package Killer — diff2test vs RTS++ / Ekstazi++

## Claim

`diff2test` is submitted for the **Package Killer** bonus as a zero-third-party-runtime-dependency replacement for the narrow CMake/CTest regression-test-selection workflow that would otherwise call for a dedicated C++ RTS tool.

The primary comparison target is **RTS++ / Ekstazi++** (`EngineeringSoftware/ekstazipp`), an open-source Regression Test Selection tool for C++.

This is intentionally **not** a claim that `diff2test` is a drop-in replacement for every RTS++ capability. The replacement claim is narrower and auditable:

> For supported Linux CMake/CTest projects whose dependency evidence is already available as CMake File API data, GCC/Clang `.d` files, and CTest JSON, `diff2test` performs conservative changed-file-to-test selection without installing or linking a dedicated RTS framework, LLVM-based analysis stack, runtime package, service, or vendored dependency.

## Why RTS++ is a legitimate comparison target

RTS++ describes itself as a **Regression Test Selection tool for C++** and is associated with the 2019 ICST paper *Resurgence of Regression Test Selection for C++*.

Its repository build configuration demonstrates that it is an installable CMake package/tool rather than merely a conceptual reference implementation:

- `find_package(LLVM REQUIRED CONFIG)` is required;
- an LLVM module/pass is built;
- an `ekstazi-lib` library is built;
- analyzer executables are built;
- CMake package configuration/export files are installed;
- the README separately points to SHA-512 source from another repository as a dependency.

Sources:

- https://github.com/EngineeringSoftware/ekstazipp
- https://github.com/EngineeringSoftware/ekstazipp/blob/main/CMakeLists.txt

## What package/tooling is being replaced

The shared problem class is **regression/test-impact selection for C++**:

```text
code change -> dependency/impact analysis -> tests that need to run
```

RTS++ solves that class of problem with a dedicated C++ RTS implementation built around LLVM-oriented analysis and its own dependency graph/test-framework integration.

`diff2test` solves a deliberately narrower CMake/CTest version of the same problem by reusing metadata the existing build already produced and parsing it in-process with C++20 standard-library facilities.

## Side-by-side comparison

| Capability | RTS++ / Ekstazi++ | diff2test |
|---|---|---|
| Primary domain | C++ regression test selection | C++ test-impact / regression test selection for supported CMake/CTest projects |
| Dedicated RTS library/tool install | yes | no third-party runtime package |
| LLVM dependency | CMake requires `LLVM` | none |
| Separate RTS library | builds `ekstazi-lib` | none; one runtime source file |
| Instrumentation/analysis integration | LLVM pass and project-specific RTS components | no instrumentation; consumes pre-generated build/test metadata |
| Test-framework integration | repository includes GoogleTest-oriented adapter code | consumes CTest's exported registered-test catalogue |
| Dependency graph source | RTS++'s own analysis machinery | compiler `.d` prerequisites + CMake File API target graph |
| Changed-file input | RTS workflow-specific | explicit newline-delimited paths/stdin |
| Selection explanation | not the focus of this comparison | concrete evidence chain for every selected test |
| Missing/ambiguous evidence | implementation-specific | explicit conservative full-suite fallback |
| Runtime subprocesses | not part of this comparison | none; source audit enforced in CI |
| Runtime third-party packages | dedicated RTS stack / LLVM-oriented build dependency | zero third-party runtime code |
| Implementation layout | multiple libraries/sources/tools | one runtime implementation file, `diff2test.cpp` |
| Supported breadth | broader research RTS architecture | intentionally narrow Linux + CMake/CTest + GCC/Clang Make-style `.d` MVP |

## Standard-library substitutions that make the replacement possible

Instead of bringing in a dedicated RTS stack, `diff2test` builds the required narrow workflow from C++20 primitives:

1. **CMake/CTest JSON parsing** — custom strict recursive-descent parser using `std::variant`, `std::map`, `std::vector`, `std::string`.
2. **Compiler dependency parsing** — custom Make-style `.d` scanner using strings/streams/containers.
3. **Filesystem identity and containment** — `std::filesystem` plus explicit lexical root policy.
4. **Dependency graph representation** — ordered standard containers rather than a graph library.
5. **Impact propagation** — `std::queue`, sets, and predecessor maps.
6. **Test mapping** — exact normalized filesystem-path equality rather than package-specific framework hooks.
7. **Result/error handling** — project structs plus `std::optional`.
8. **CLI parsing** — direct `argc`/`argv` parsing.
9. **Deterministic formatting** — standard streams/algorithms/ordered containers.
10. **Safety/completeness ledger** — explicit in-process policy rather than assuming unavailable evidence is harmless.

The full substitution inventory is in [`STDLIB.md`](STDLIB.md).

## Why metadata reuse matters

A package-free implementation only works because `diff2test` deliberately avoids re-implementing a compiler or build system. It consumes **data already emitted by tools the project normally uses**:

- CMake File API codemodel v2;
- GCC/Clang Make-style `.d` files;
- CTest `ctestInfo` JSON v1;
- changed paths supplied by the caller.

CMake, CTest, Git, and the compiler remain external build/input-generation tooling. `diff2test` never invokes them at runtime.

This distinction was explicitly approved by the hackathon organizer and is documented in [`ORGANIZER-CLARIFICATIONS.md`](ORGANIZER-CLARIFICATIONS.md).

## What diff2test deliberately does *not* replace

The Package Killer claim should not be read beyond the tested MVP boundary. `diff2test` does not claim to replace RTS++ for:

- arbitrary C++ build systems;
- LLVM instrumentation workflows;
- arbitrary test frameworks outside the direct CTest executable mapping model;
- Windows/MSVC dependency formats;
- Ninja dependency databases;
- generated/custom-command dependency chains;
- history/coverage/dynamic RTS strategies;
- broad research-tool experimentation.

Those are real capabilities/scope differences, not omissions hidden from the comparison.

## Evidence that the replacement workflow actually works

Public CI verifies the complete supported workflow with real generated metadata:

- `include/alpha.hpp` selects exactly `AlphaTest`;
- a shared feature header selects exactly `AlphaTest` and `BetaTest`;
- missing dependency evidence widens to all known tests with exit `10`;
- missing CTest catalogue emits no invented names and returns exit `11`;
- reordered evidence and repeated runs remain byte-identical;
- all tests pass under ASan/UBSan;
- the runtime source contains no process-spawn API;
- two clean same-runner Release builds are byte-identical.

See [`FINAL-AUDIT.md`](FINAL-AUDIT.md), [`DEPENDENCY-PROOF.md`](DEPENDENCY-PROOF.md), and `.github/workflows/ci.yml`.

## Bottom line

RTS++ demonstrates that C++ regression-test selection is substantial enough to justify a dedicated toolchain/library architecture. `diff2test` attacks the same core workflow from the opposite direction: **use trustworthy metadata already present in a CMake/CTest build, implement only the required analysis in standard C++20, and fall back whenever that evidence is insufficient.**

For that deliberately supported use case, the extra RTS package/tool stack is removed from the runtime path entirely.
