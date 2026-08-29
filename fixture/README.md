# Controlled metadata fixture

This fixture validates `diff2test` against the supported CMake/CTest/GCC-or-Clang metadata shape. It is development/demo material, not part of the runtime artifact.

## Generate File API replies correctly

The CMake File API query belongs in the **build tree**, before configure:

```bash
cmake -E make_directory fixture/build/.cmake/api/v1/query
cmake -E touch fixture/build/.cmake/api/v1/query/codemodel-v2
cmake -S fixture -B fixture/build -DCMAKE_BUILD_TYPE=Debug
cmake --build fixture/build
ctest --test-dir fixture/build --show-only=json-v1 > fixture/build/ctest-info.json
find fixture/build -type f -name '*.o.d' -printf '%P\n' | sort > fixture/build/deps.txt
```

These are external developer/workflow commands. `diff2test` never launches CMake, CTest, Git, a compiler, a shell, or another executable.

## Tests in the fixture

- `AlphaTest` -> `alpha_test`
- `BetaTest` -> `beta_test`
- `CoreTest` -> `core_test`

Relationships:

- `alpha` depends on `core`;
- `beta` depends on `core`;
- `include/alpha.hpp` is specific to the alpha feature path;
- `include/beta.hpp` is specific to the beta feature path;
- `include/features_shared.hpp` is compiled by alpha and beta, but not core;
- `include/common.hpp` participates in the shared core/feature graph.

## Verified scenarios

### Narrow change

```bash
printf 'include/alpha.hpp\n' > fixture/changed.txt
```

Expected names output:

```text
AlphaTest
```

### Shared feature change

```bash
printf 'include/features_shared.hpp\n' > fixture/changed-shared.txt
```

Expected names output:

```text
AlphaTest
BetaTest
```

### Missing dependency evidence

Remove the `alpha` compile dependency entry from a copy of `deps.txt`. The expected outcome is `FULL_SUITE_SELECTED` / exit `10`, with all known tests:

```text
AlphaTest
BetaTest
CoreTest
```

### Missing catalogue

Point `--ctest-info` at an absent file. The expected outcome is `FULL_SUITE_REQUIRED` / exit `11`, with no invented names.

## What the fixture proves

1. exact CTest command executable <-> File API executable artifact mapping;
2. compiler `.d` prerequisite syntax and translation-unit evidence;
3. narrow single-feature selection;
4. shared-header multi-test selection;
5. reverse target-impact propagation;
6. deterministic output;
7. conservative full-suite fallback after evidence removal;
8. catalogue failure without invented test names.

The public CI executes these scenarios using metadata generated externally during the run.

Do not treat this fixture as proof of support for arbitrary CMake generators, wrapper commands, generated/custom-command chains, Windows/MSVC dependency formats, or absolutely fresh/adversarial metadata.
