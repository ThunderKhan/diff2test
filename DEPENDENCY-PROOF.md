# Dependency Proof — diff2test

## Claim

`diff2test` ships with **zero third-party runtime dependencies** and contains no vendored third-party source. The runtime implementation is the single C++20 file `diff2test.cpp`.

The executable uses normal C++/toolchain/system runtime libraries permitted by the event rules. It does not launch separately installed tools or contact a service.

CMake is **permitted external build/input-generation tooling, not a third-party runtime dependency of `diff2test`**.

## Verified environment

Public GitHub Actions verification on 2026-08-29 used:

- GitHub runner image: Ubuntu 24.04.4 LTS (`ubuntu-24.04`, image `20260823.283.1`);
- architecture: x86-64;
- GNU C++: 13.3.0 (selected by CMake in the verification log);
- CMake available on that runner image: 3.31.6;
- C++ standard: C++20;
- normal build warning set: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`;
- Release reproducibility check: `CMAKE_BUILD_TYPE=Release`, `BUILD_TESTING=OFF`.

The key verification run for the expanded clean-room/CLI/demo checks is GitHub Actions run `33233072308`.

## Build-tool/runtime boundary

CMake has two external roles in this repository:

1. it may configure/build the `diff2test` executable;
2. it may generate File API data for a CMake project before analysis.

CTest may export `--show-only=json-v1`; the compiler/build may produce `.d` files; Git may produce a changed-path list.

None of those programs is launched by the built `diff2test` process. The program only opens/parses files or stdin that the caller supplies.

If metadata is absent or untrusted, `diff2test` returns a conservative safety outcome rather than invoking a producer:

- unusable CMake/dependency evidence with a trusted CTest catalogue -> `FULL_SUITE_SELECTED`;
- unusable CTest catalogue -> `FULL_SUITE_REQUIRED` with no invented test names.

## Clean build

A judge can reproduce the normal Release artifact with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
```

Produced artifact:

```text
build/diff2test
```

The repository `CMakeLists.txt` only creates the executable/test targets and warning flags. It contains no `FetchContent`, package-manager invocation, dependency download, vendored library build, or third-party runtime package declaration.

## Repository inventory

Relevant runtime/development separation:

```text
diff2test.cpp             runtime implementation (single source file)
CMakeLists.txt             build/test configuration
tests/                     original dependency-free test executables
fixture/                   controlled metadata-generation/integration project
.github/workflows/ci.yml   dev/build/proof automation
*.md                       documentation/contracts/evidence
```

There is no `vendor/`, `third_party/`, runtime lockfile, submodule, generated library source bundle, or packaged third-party application library in the project.

## Dynamic runtime linkage

The Linux CI proof runs:

```bash
ldd build/diff2test
```

Observed dynamic entries in run `33233072308`:

```text
linux-vdso.so.1
libstdc++.so.6
libgcc_s.so.1
libc.so.6
libm.so.6
/lib64/ld-linux-x86-64.so.2
```

These are the Linux/toolchain/system runtime components expected for a normally linked GNU C++ executable. No CMake library and no third-party application library is present.

The claim is intentionally **not** “fully static binary.” It is “zero third-party runtime dependency” under the event’s C/C++ runtime rules.

## No subprocess proof

CI searches the runtime implementation for common process-spawn APIs:

```bash
grep -nE '\b(system|popen|pclose|fork|exec[lvpe]*|posix_spawn)\s*\(' diff2test.cpp
```

The verification step succeeds only when this search finds no runtime process-spawn call.

The current implementation therefore has no code path that invokes:

- CMake;
- CTest;
- Git;
- a compiler;
- Python or another interpreter;
- a shell;
- another executable.

This source audit is supplemented by the architecture itself: the analysis pipeline uses `std::filesystem`, file streams, strings/containers, and in-process parsers/graph logic only.

## No network/service requirement

`diff2test.cpp` contains no networking client, socket workflow, HTTP library, service URL, authentication configuration, telemetry path, or plugin loader. Runtime analysis is local-file/stdin processing.

GitHub Actions itself is development verification infrastructure and is not part of the runtime artifact.

## External metadata boundary

The supported inputs are ordinary data:

| Data | External producer/example | What `diff2test` does |
|---|---|---|
| changed paths | file, CI, or `git diff --name-only` piped externally | reads lines from file/stdin |
| CMake File API reply | external CMake configure | parses supported JSON files |
| compiler `.d` files | external GCC/Clang build | parses Make-style dependency rules |
| CTest JSON | external `ctest --show-only=json-v1` | parses registered-test catalogue |

The organizer specifically allowed parsing pre-generated files when disclosed and when the program degrades gracefully if they are unavailable. This repository implements that boundary directly.

## Test/dev tooling does not ship as runtime code

The public CI also uses:

- CTest to execute repository tests;
- Bash/coreutils for CI assertions;
- `ldd` for proof;
- GCC AddressSanitizer and UndefinedBehaviorSanitizer in a separate build.

The sanitizer build is a dev-only binary/test configuration. The normal Release artifact does not use sanitizer flags or sanitizer linkage.

## Reproducible-build proof

The same CI runner/toolchain performs two independent clean Release configurations/builds:

```bash
cmake -S . -B release-a -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build release-a
cmake -S . -B release-b -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build release-b
sha256sum release-a/diff2test release-b/diff2test
cmp release-a/diff2test release-b/diff2test
```

Run `33233072308` produced the same SHA-256 for both artifacts:

```text
162a6bbf52034f0c468ab2c7c82853a449590530768e9ed6ddd82f1b7aabc903
```

and `cmp` succeeded. This is a **same-environment/toolchain reproducibility result**, not a cross-platform/cross-toolchain reproducibility claim.

## Clean-room functional verification

The same run also proved, from a fresh CI checkout:

- configure/build succeeds;
- all seven repository test executables pass;
- CLI usage/error semantics pass;
- real CMake/compiler/CTest metadata is generated externally;
- `include/alpha.hpp` selects exactly `AlphaTest`;
- stdin/file changed-path input produces the same result;
- `include/features_shared.hpp` selects exactly `AlphaTest` and `BetaTest`;
- reordered evidence and 20 repeated runs remain byte-stable;
- missing dependency evidence widens to all known tests with exit `10`;
- missing CTest catalogue emits no names and exits `11`;
- empty changed input exits `64`;
- runtime process-spawn source audit passes;
- dynamic-link inspection shows only system/toolchain runtime entries;
- two Release builds are byte-identical.

The separate sanitizer job in the same run also passes all seven tests under ASan/UBSan.

## Reproduction summary

Build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
```

Inspect runtime linkage:

```bash
ldd build/diff2test
```

Audit subprocess APIs:

```bash
grep -nE '\b(system|popen|pclose|fork|exec[lvpe]*|posix_spawn)\s*\(' diff2test.cpp
```

Run tests:

```bash
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

Generate the controlled analysis inputs and run the smoke scenario using the commands in [`README.md`](README.md).

## Result

**PASS** for the verified Linux/GNU C++/CMake MVP environment:

- zero third-party runtime application libraries observed;
- no CMake runtime linkage;
- no runtime subprocess APIs found;
- no runtime service/network requirement;
- external metadata explicitly disclosed;
- missing metadata degrades conservatively;
- same-runner clean Release builds are byte-identical.

For the concrete standard-library/package substitutions, see [`STDLIB.md`](STDLIB.md).
