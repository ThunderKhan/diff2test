# diff2test

**A zero-third-party-runtime-dependency C++20 test-impact analyzer for CMake/CTest projects.**

Built for the Zero Dependency Hackathon — Track A: Developer Tools & CLI, team `std::zero`.

`diff2test` answers one question conservatively: **given a set of changed project paths, which registered CTest tests can be justified as affected by metadata the build already produced?** If the evidence is incomplete, malformed, stale, unsupported, or ambiguous, it refuses to guess and widens to the full known suite.

## Why

Running every C++ test is safe but can be slow. Naive test selection is faster but dangerous: a false negative can hide a regression. CMake, compilers, and CTest already emit useful structural evidence, but that evidence is fragmented across formats.

`diff2test` connects those formats without invoking the tools that produced them:

```text
changed path
  -> compiler dependency evidence (.d)
  -> translation unit
  -> CMake target
  -> dependent target(s)
  -> executable artifact
  -> registered CTest test
```

The safety rule is intentionally asymmetric:

> A false positive costs time. A false negative can hide a bug. When evidence is uncertain, run everything.

## What it consumes

All inputs are generated or supplied **before** `diff2test` runs:

- newline-delimited changed paths;
- a CMake File API reply directory containing codemodel v2 data;
- an explicit list of GCC/Clang Make-style `.d` files;
- CTest `--show-only=json-v1` output.

`diff2test` does **not** run tests and does **not** launch Git, CMake, CTest, a compiler, Python, a shell, or another external executable.

## 30-second example

After generating the fixture metadata as shown below:

```bash
printf 'include/alpha.hpp\n' > fixture/changed.txt

./build/diff2test analyze \
  --project-root "$(pwd)/fixture" \
  --build-root "$(pwd)/fixture/build" \
  --changed-files fixture/changed.txt \
  --cmake-reply fixture/build/.cmake/api/v1/reply \
  --ctest-info fixture/build/ctest-info.json \
  --dep-list fixture/build/deps.txt \
  --format names
```

Verified output:

```text
AlphaTest
```

A real CI integration test also changes `include/features_shared.hpp` and verifies that exactly these two tests are selected:

```text
AlphaTest
BetaTest
```

Removing required dependency evidence causes exit status `10` and emits the full known fixture suite instead:

```text
AlphaTest
BetaTest
CoreTest
```

## Supported MVP boundary

The supported, tested MVP is deliberately narrow:

- Linux;
- C++20;
- GCC or Clang producing Make-style compiler dependency files;
- CMake File API codemodel major version 2;
- CTest `ctestInfo` JSON major version 1;
- one explicit or unambiguous CMake configuration;
- CMake Unix Makefiles-style object/dependency layout matching `CMakeFiles/<target>.dir/...`;
- direct CTest executable commands that map exactly to one CMake executable artifact;
- UTF-8/ASCII project paths with no embedded NUL/newline.

Unknown or unsupported shapes do not trigger heuristic matching; they trigger conservative fallback.

## Build

Requirements for **building the executable**:

- a C++20 compiler;
- CMake 3.20+ as build tooling.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
```

Artifact:

```text
build/diff2test
```

To build and run the repository test suite:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### Is CMake a dependency?

CMake is **permitted external build/input-generation tooling, not a third-party runtime dependency of `diff2test`**.

The organizer explicitly clarified that the compiler/build tool does not count against the runtime-dependency rule, and that parsing files those tools already produced is permitted when disclosed and handled gracefully when absent.

CMake is used externally to build this repository and may be used externally to generate File API metadata. The built `diff2test` process never invokes CMake or links a CMake library. If required CMake/CTest-generated data is unavailable, the program falls back rather than launching the missing producer.

See [`ORGANIZER-CLARIFICATIONS.md`](ORGANIZER-CLARIFICATIONS.md), [`STDLIB.md`](STDLIB.md), and [`DEPENDENCY-PROOF.md`](DEPENDENCY-PROOF.md).

## Generate analysis inputs externally

The following commands are **developer/user workflow commands**. They are not executed by `diff2test`.

### 1. Request the CMake File API codemodel

The query marker belongs in the build tree before configure:

```bash
cmake -E make_directory fixture/build/.cmake/api/v1/query
cmake -E touch fixture/build/.cmake/api/v1/query/codemodel-v2
```

### 2. Configure and build

```bash
cmake -S fixture -B fixture/build -DCMAKE_BUILD_TYPE=Debug
cmake --build fixture/build
```

### 3. Export CTest metadata

```bash
ctest --test-dir fixture/build --show-only=json-v1 > fixture/build/ctest-info.json
```

### 4. Produce the explicit dependency-file list

For the controlled Unix Makefiles fixture:

```bash
find fixture/build -type f -name '*.o.d' -printf '%P\n' | sort > fixture/build/deps.txt
```

The explicit list is intentional. The kickoff spike observed unrelated `.d` files such as `link.d`; recursively treating every `.d` file as compiler dependency evidence would be unsafe.

### 5. Supply changed paths

From a plain file:

```bash
printf 'include/alpha.hpp\n' > fixture/changed.txt
```

Or externally from Git:

```bash
git diff --name-only origin/main...HEAD | ./build/diff2test analyze \
  --project-root "$(pwd)" \
  --build-root "$(pwd)/build" \
  --changed-files - \
  --cmake-reply build/.cmake/api/v1/reply \
  --ctest-info build/ctest-info.json \
  --dep-list build/deps.txt \
  --format names
```

Git is outside the process. `diff2test` only reads stdin.

## CLI

```text
diff2test analyze \
  --project-root <dir> \
  --build-root <dir> \
  --changed-files <file|-> \
  --cmake-reply <dir> \
  --ctest-info <file> \
  --dep-list <file> \
  [--cmake-index <file>] \
  [--configuration <name>] \
  [--format human|names] \
  [--explain] \
  [--verbose]
```

Useful commands:

```bash
./build/diff2test --help
./build/diff2test --version
```

### Output modes

`--format names` emits only selected test names on stdout, one per line. Diagnostics and fallback reasons go to stderr.

`--format human` emits a readable report. Add `--explain` to show the evidence chain for each selected test.

## Safety outcomes and exit statuses

| Exit | Outcome | Meaning |
|---:|---|---|
| `0` | `SUBSET_SELECTED` | complete supported evidence justified the emitted selection |
| `10` | `FULL_SUITE_SELECTED` | catalogue is known, but other evidence is unsafe; all known tests are emitted |
| `11` | `FULL_SUITE_REQUIRED` | test catalogue cannot be trusted/enumerated; no test names are invented |
| `64` | `USAGE_ERROR` | invalid invocation or empty changed-path input |
| `65` | `INPUT_ERROR` | required input failed in a context where no safe enumerated result is available |
| `70` | `INTERNAL_ERROR` | unexpected invariant/exception boundary |

Exit `10` and `11` are deliberately nonzero so CI integrations cannot silently overlook a fallback state.

## Safety behavior

A proper selective result is committed only after the complete evidence audit passes. Examples that prevent a narrow result include:

- missing/malformed/unsupported CTest catalogue;
- missing or ambiguous File API reply/configuration;
- target-reference or root mismatch;
- missing, duplicate, malformed, or detectably stale `.d` evidence;
- wrapper/interpreter-style CTest commands;
- zero/multiple executable-artifact matches;
- generated source/custom-command relationships outside the MVP model;
- unknown changed project paths;
- changed `CMakeLists.txt` or `.cmake` configuration input;
- unknown target dependency edges.

Detectable staleness is checked by comparing required project-prerequisite timestamps with their `.d` files. Passing this check means **no staleness was detected under that policy**, not that metadata is cryptographically bound to source contents.

For the detailed rules, see [`SAFETY-CONTRACT.md`](SAFETY-CONTRACT.md).

## Why exact mapping instead of heuristics?

CTest executable mapping uses normalized artifact paths. Basename-only matching and naming conventions such as `*_test` are intentionally rejected because two unrelated artifacts can share a basename or a wrapper may alter the true executable boundary.

Similarly, unknown changed paths are not ignored by extension: a `.md`, `.txt`, or generated input can still affect a test through tooling not modeled by the MVP.

## Verification

The public CI currently verifies:

- seven C++ test executables;
- strict JSON parser cases and resource limits;
- a 10,000-prerequisite dependency-parser stress case;
- path/root-containment cases;
- CTest and CMake metadata loader cases;
- safety mutation cases;
- detectably stale `.d` fallback;
- target chain, diamond, and cycle traversal;
- metadata/input-order invariance;
- byte-identical stdout/stderr across reordered evidence and 20 repeated real-fixture analyses;
- real `alpha.hpp -> AlphaTest` selection;
- real `features_shared.hpp -> AlphaTest + BetaTest` selection;
- real missing-evidence full-suite fallback;
- missing-catalogue `FULL_SUITE_REQUIRED` behavior;
- stdin/file changed-path equivalence;
- CLI usage-error semantics;
- source audit for runtime process-spawn APIs;
- Linux dynamic-link inspection;
- AddressSanitizer + UndefinedBehaviorSanitizer test runs;
- two clean same-runner Release builds compared byte-for-byte.

## Zero-dependency design

Runtime implementation is a single source file:

```text
diff2test.cpp
```

There is no vendored third-party source and no runtime package manager/library dependency. Tests, fixtures, build scripts, CI, and documentation are separate development/submission material.

The implementation replaces common packages with C++20 standard-library facilities and purpose-built parsers/graph logic. See [`STDLIB.md`](STDLIB.md) for the concrete substitution log and [`DEPENDENCY-PROOF.md`](DEPENDENCY-PROOF.md) for the runtime-dependency audit.

## Project structure

```text
diff2test.cpp             single runtime implementation source
CMakeLists.txt             build/test configuration
tests/                     repository tests; not runtime code
fixture/                   controlled CMake/CTest integration fixture
.github/workflows/ci.yml   development verification
STDLIB.md                  zero-dependency substitution log
DEPENDENCY-PROOF.md        runtime dependency evidence and reproduction
SAFETY-CONTRACT.md         conservative selection rules
INPUT-SPEC.md              supported input formats/boundaries
CLI-CONTRACT.md            command/output/exit contract
WORKLOG.md                 in-window implementation record
```

## Tradeoffs and limitations

The design intentionally chooses auditability and safe fallback over breadth.

Not claimed in the MVP:

- Windows/MSVC dependency formats;
- Ninja `.ninja_deps`;
- recursive `.d` auto-discovery across arbitrary generators;
- wrapper/interpreter test commands;
- generated/custom-command dependency chains;
- CMake-language interpretation;
- raw Git patch parsing;
- coverage/history/ML selection;
- absolute freshness guarantees;
- executing the selected tests.

A mature third-party JSON/CLI/graph stack would provide broader format support and conveniences. The point of this entry is to show how far the C++ standard library can go while preserving an explicit safety model.

## Development-tool boundary

CMake, CTest, Git, compilers, `ldd`, sanitizers, and shell commands in CI are development/build/proof tooling. None are invoked by the runtime artifact. The event clarification and this repository consistently distinguish those tools from third-party runtime dependencies.

## License

MIT. See [`LICENSE`](LICENSE).

## Evidence and references

Primary format references and organizer-rule notes are recorded in [`SOURCES.md`](SOURCES.md), [`SPIKE-RESULTS.md`](SPIKE-RESULTS.md), and [`ORGANIZER-CLARIFICATIONS.md`](ORGANIZER-CLARIFICATIONS.md).
