<div align="center">

<img src="assets/diff2test-hero.png" alt="diff2test — zero-runtime-dependency test impact analysis for CMake/CTest" width="100%" />

<br />

[![CI](https://github.com/ThunderKhan/diff2test/actions/workflows/ci.yml/badge.svg)](https://github.com/ThunderKhan/diff2test/actions/workflows/ci.yml)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Zero Runtime Dependencies](https://img.shields.io/badge/runtime%20dependencies-zero-00b894)
![Single File](https://img.shields.io/badge/runtime%20implementation-single%20file-6c5ce7)
![License](https://img.shields.io/badge/license-MIT-2ea44f)

Select only the tests your build evidence can actually justify — and widen safely when that evidence is incomplete.

**Zero Dependency Hackathon · Track A: Developer Tools & CLI · `std::zero`**

[Quick start](#quick-start) · [How it works](#how-it-works) · [Safety model](#safety-model) · [Verification](#verification) · [Package Killer](#package-killer) · [Docs](#documentation)

</div>

---

## Why diff2test?

Large C++ projects often face a bad tradeoff:

- **Run everything** — safe, but potentially expensive.
- **Guess what changed** — fast, but a false negative can hide a regression.

`diff2test` takes a stricter approach:

> **A test is omitted only when the available build evidence justifies omitting it.**

It reads metadata your existing toolchain already produced, builds an explainable impact graph, and selects the affected CTest tests. If the evidence becomes missing, malformed, stale, ambiguous, or unsupported, the result widens instead of guessing.

### What it does not do

`diff2test` does **not** run tests and does **not** launch Git, CMake, CTest, a compiler, Python, a shell, a network service, or another executable at runtime.

---

## How it works

<p align="center">
  <img src="assets/diff2test-workflow.png" alt="How diff2test maps changed paths to affected CTest tests" width="100%" />
</p>

```text
changed path
    ↓
compiler dependency evidence (.d)
    ↓
translation unit
    ↓
CMake target
    ↓
dependent target(s)
    ↓
executable artifact
    ↓
registered CTest test
```

The supported analysis combines four pre-generated inputs:

| Evidence | Purpose |
|---|---|
| changed paths | what the caller says changed |
| CMake File API codemodel v2 | sources, targets, artifacts, target dependencies |
| GCC/Clang Make-style `.d` files | file/header → translation-unit dependency evidence |
| CTest `json-v1` catalogue | registered tests and their command executables |

For every selected test, `--explain` can show a concrete evidence chain from the changed path to the registered test.

---

## Quick start

> **diff2test never runs Git, CMake, or CTest; it consumes changed paths and metadata those tools already produced.**

For the conventional `build/` layout, generate the catalogue and dependency-file list externally, then compose changed paths through stdin:

```bash
ctest --test-dir build --show-only=json-v1 > build/ctest-info.json
find build -type f -name '*.o.d' -printf '%P\n' | sort > build/deps.txt
git diff --name-only HEAD~1 | ./build/diff2test analyze .
```

The shell launches `git`, `ctest`, and `find`; `diff2test` launches nothing. The shorthand defaults to `./build`, stdin, `build/.cmake/api/v1/reply`, `build/ctest-info.json`, and `build/deps.txt`. Every default has an explicit override.


> **diff2test never runs Git, CMake, or CTest; it consumes changed paths and metadata those tools already produced.**

### 1. Build `diff2test`

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
```

The executable is produced at:

```text
build/diff2test
```

### 2. Generate metadata externally

These commands are part of the caller/developer workflow. They are **not** executed by `diff2test`.

```bash
cmake -E make_directory build/.cmake/api/v1/query
cmake -E touch build/.cmake/api/v1/query/codemodel-v2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --show-only=json-v1 > build/ctest-info.json
find build -type f -name '*.o.d' -printf '%P\n' | sort > build/deps.txt
```

The `.d` list is generated externally because `diff2test` deliberately does not recursively accept every `.d` file under a build tree. The analysis engine still validates each listed dependency file against known CMake targets and translation units.

### 3. Analyze changed paths

Preferred common case:

```bash
git diff --name-only HEAD~1 | ./build/diff2test analyze .
```

With a conventional `build/` directory, that shorthand means:

```text
project root   = .
build root     = ./build
changed paths  = stdin
CMake reply    = ./build/.cmake/api/v1/reply
CTest catalogue= ./build/ctest-info.json
dependency list= ./build/deps.txt
```

The shell launches `git` and creates the pipe. `diff2test` only reads stdin and existing files.

Explicit overrides remain available:

```bash
# Explicit build directory
git diff --name-only HEAD~1 | ./build/diff2test analyze . --build build-debug

# Explicit changed-file source
./build/diff2test analyze . --changed-files changed.txt

# Fully explicit legacy/advanced form
./build/diff2test analyze \
  --project-root . \
  --build-root build \
  --changed-files changed.txt \
  --cmake-reply build/.cmake/api/v1/reply \
  --ctest-info build/ctest-info.json \
  --dep-list build/deps.txt
```

For the controlled fixture, changing `include/alpha.hpp` selects:

```text
AlphaTest
```

A shared-header change selects exactly:

```text
AlphaTest
BetaTest
```

If required dependency evidence is missing or unsafe, the tool returns exit `10` and emits the full known fixture suite:

```text
AlphaTest
BetaTest
CoreTest
```

---

## Safety model

The optimization is conditional. The fallback is not.

<p align="center">
  <img src="assets/diff2test-safety.png" alt="diff2test conservative safety outcomes" width="100%" />
</p>

| Exit | Outcome | Meaning |
|---:|---|---|
| `0` | `SUBSET_SELECTED` | complete supported evidence justified the emitted subset |
| `10` | `FULL_SUITE_SELECTED` | test catalogue is trusted, but other evidence is unsafe; emit every known test |
| `11` | `FULL_SUITE_REQUIRED` | test catalogue cannot be trusted/enumerated; emit no invented names |
| `64` | `USAGE_ERROR` | invalid invocation or empty changed-path input |
| `65` | `INPUT_ERROR` | required input failed where no safe enumerated result can be produced |
| `70` | `INTERNAL_ERROR` | unexpected invariant/exception boundary |

`10` and `11` are intentionally non-zero so CI integration cannot silently overlook a safety fallback.

### Cases that prevent narrow selection

Examples include:

- missing, malformed, or unsupported CTest metadata;
- missing or ambiguous CMake File API replies/configurations;
- root or target-reference mismatches;
- missing, duplicate, malformed, or detectably stale `.d` evidence;
- wrapper/interpreter-style test commands;
- zero or multiple executable-artifact matches;
- generated/custom-command relationships outside the MVP model;
- unknown changed paths;
- changed `CMakeLists.txt` or `.cmake` files;
- unknown target dependency edges.

See [`SAFETY-CONTRACT.md`](SAFETY-CONTRACT.md) for the complete policy.

---

## Explainable by construction

Human mode can expose the evidence used for a selection:

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

The important property is not just *what* was selected, but *why that selection was permitted*.

---

## Supported MVP boundary

The current verified boundary is intentionally narrow:

- Linux;
- C++20;
- GCC or Clang producing Make-style compiler dependency files;
- CMake File API codemodel major version 2;
- CTest `ctestInfo` JSON major version 1;
- one explicit or unambiguous CMake configuration;
- CMake Unix Makefiles-style dependency-target layout matching `CMakeFiles/<target>.dir/...`;
- direct CTest executable commands that map exactly to one CMake executable artifact;
- UTF-8/ASCII project paths without embedded NUL/newline.

Unsupported shapes trigger conservative fallback rather than heuristic matching.

<details>
<summary><strong>Why Linux / Unix Makefiles only for the MVP?</strong></summary>

The core graph algorithm is not inherently Linux-only. The platform-specific boundary is the dependency evidence and build layout being interpreted safely.

Windows/MSVC, Ninja, and other generators can expose different dependency formats, path semantics, object layouts, or databases. Supporting those honestly requires a separately tested evidence adapter rather than assuming the current `.d` mapping generalizes.

</details>

---

## CLI

```text
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

Defaults:

```text
project-root    .
build-root      <project-root>/build
changed-files   stdin
cmake-reply     <build-root>/.cmake/api/v1/reply
ctest-info      <build-root>/ctest-info.json
dep-list        <build-root>/deps.txt
```

`--build` is a convenience alias for `--build-root`. Supplying both is a usage error. Supplying both a positional project root and `--project-root` is also a usage error.

Useful commands:

```bash
./build/diff2test --help
./build/diff2test --version
```

### Machine-friendly use with Git

Git can feed changed paths externally through stdin:

```bash
git diff --name-only origin/main...HEAD | ./build/diff2test analyze . --format names
```

Git is run by the caller's workflow; `diff2test` only reads stdin.

---

## Zero-runtime-dependency design

The entire runtime implementation lives in one source file:

```text
diff2test.cpp
```

No vendored third-party runtime source, package manager, service client, plugin system, or runtime subprocess mechanism is used.

Capabilities commonly delegated to libraries were implemented with C++20 standard-library primitives and purpose-built code, including:

- CLI parsing;
- strict JSON parsing;
- UTF-8 and JSON Unicode-escape handling;
- filesystem/path containment;
- Make-style `.d` parsing;
- graph representation and traversal;
- deterministic formatting;
- error/result handling;
- repository test harnesses.

See [`STDLIB.md`](STDLIB.md) for the full substitution log and [`DEPENDENCY-PROOF.md`](DEPENDENCY-PROOF.md) for the runtime dependency audit.

### Is CMake a dependency?

CMake is **external build/input-generation tooling, not a third-party runtime dependency of the shipped executable**.

The organizer explicitly clarified that build tools are permitted and that pre-generated CMake/CTest output may be parsed when disclosed and handled gracefully when absent. `diff2test` never launches those tools itself.

---

## Verification

The public CI verifies much more than a happy path:

- **7** dependency-free C++ test executables;
- strict JSON grammar, UTF-8, Unicode and resource-boundary cases;
- a **10,000-prerequisite** dependency-parser stress case;
- lexical path/root containment and escape cases;
- CMake and CTest metadata validation;
- missing/malformed/duplicate/stale/ambiguous evidence mutations;
- chain, diamond, cycle and unaffected graph shapes;
- exact executable-artifact mapping;
- real `alpha.hpp → AlphaTest` selection;
- real shared-header `→ AlphaTest + BetaTest` selection;
- shorthand/default CLI and fully explicit CLI equivalence;
- default stdin changed-path input;
- explicit override behavior, including `--build`;
- real missing-evidence full-suite fallback;
- missing-catalogue `FULL_SUITE_REQUIRED` behavior;
- stdin/file input equivalence;
- byte-identical output under reordered evidence;
- **20 repeated real-fixture analyses** with byte-stable stdout/stderr;
- runtime process-spawn source audit;
- Linux dynamic-link inspection;
- separate ASan + UBSan test builds;
- two independent same-runner Release builds compared byte-for-byte.

Reproducible same-runner Release SHA-256 from the verified build:

```text
162a6bbf52034f0c468ab2c7c82853a449590530768e9ed6ddd82f1b7aabc903
```

---

## Hackathon bonus evidence

| Bonus | Evidence |
|---|---|
| **Single File · +5** | runtime implementation is only `diff2test.cpp` |
| **Reproducible Build · +5** | two clean same-runner Release binaries compared byte-for-byte |
| **STDLIB Log · +3** | `STDLIB.md` documents 10+ genuine package/category substitutions |
| **Package Killer · +3 claimed** | `PACKAGE-KILLER.md` compares the supported RTS use case against RTS++ / Ekstazi++ |

### Package Killer

The primary comparison target is **RTS++ / Ekstazi++**, an open-source Regression Test Selection tool for C++ whose build uses LLVM, an LLVM pass, `ekstazi-lib`, CMake package/export files, and separately sourced SHA-512 implementation code.

For the narrower supported CMake/CTest workflow, `diff2test` replaces the dedicated RTS package/tool stack in the runtime path by consuming existing build metadata and implementing the necessary parsing, mapping, traversal, explanation, and conservative fallback in one C++20 source file.

This is deliberately **not** a drop-in-equivalence claim for every RTS++ capability.

See [`PACKAGE-KILLER.md`](PACKAGE-KILLER.md) for the evidence-backed comparison.

---

## Project structure

```text
diff2test.cpp             single runtime implementation source
CMakeLists.txt             build/test configuration
tests/                     dependency-free repository tests
fixture/                   controlled CMake/CTest integration fixture
.github/workflows/ci.yml   public verification
README.md                  product overview and quick start
STDLIB.md                  zero-dependency substitution log
PACKAGE-KILLER.md          C++ RTS package/tool comparison
DEPENDENCY-PROOF.md        runtime dependency and reproducibility evidence
SAFETY-CONTRACT.md         conservative selection rules
INPUT-SPEC.md              supported input formats and boundaries
CLI-CONTRACT.md            command/output/exit contract
FINAL-AUDIT.md             planning-to-implementation reconciliation
WORKLOG.md                 in-window implementation record
```

---

## Documentation

| Document | What it proves / explains |
|---|---|
| [`STDLIB.md`](STDLIB.md) | what packages/categories were replaced with stdlib and purpose-built code |
| [`DEPENDENCY-PROOF.md`](DEPENDENCY-PROOF.md) | runtime linkage, process-spawn audit, reproducible build evidence |
| [`PACKAGE-KILLER.md`](PACKAGE-KILLER.md) | narrow RTS++ / Ekstazi++ Package Killer comparison |
| [`SAFETY-CONTRACT.md`](SAFETY-CONTRACT.md) | conditions required before a subset may be emitted |
| [`INPUT-SPEC.md`](INPUT-SPEC.md) | exact supported input formats and limits |
| [`CLI-CONTRACT.md`](CLI-CONTRACT.md) | commands, outputs, and stable exit statuses |
| [`DEMO-SCRIPT.md`](DEMO-SCRIPT.md) | frozen under-five-minute demo flow |
| [`FINAL-AUDIT.md`](FINAL-AUDIT.md) | final MVP completion and scope reconciliation |
| [`WORKLOG.md`](WORKLOG.md) | implementation timeline during the hackathon window |

---

## Limitations

`diff2test` intentionally does **not** claim support for:

- Windows/MSVC dependency formats;
- Ninja `.ninja_deps`;
- arbitrary CMake generators;
- recursive unsafe `.d` discovery;
- wrapper/interpreter CTest commands;
- generated/custom-command dependency chains;
- parsing CMake language itself;
- raw Git patch parsing;
- coverage/history/ML-based selection;
- absolute proof that metadata matches source contents;
- executing selected tests;
- cross-platform or cross-toolchain reproducible binaries.

Those are scope boundaries, not silent assumptions.

---

## License

Released under the [MIT License](LICENSE).

<div align="center">

**Complete evidence gives a smaller suite. Uncertainty gives everything.**

</div>
