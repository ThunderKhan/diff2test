# Controlled metadata fixture

This fixture exists only to validate diff2test's supported CMake/CTest/GCC-or-Clang metadata shape. It is not part of the runtime artifact.

## Generate File API replies correctly

The CMake File API query belongs in the **build tree**, before configure:

```bash
cmake -E make_directory fixture/build/.cmake/api/v1/query
cmake -E touch fixture/build/.cmake/api/v1/query/codemodel-v2
cmake -S fixture -B fixture/build
cmake --build fixture/build
ctest --test-dir fixture/build --show-only=json-v1 > fixture/build/ctest-info.json
```

These commands are run by the developer workflow. `diff2test` never launches CMake, CTest, a compiler, a shell, or any external executable.

## What this fixture proves

It provides three direct CTest executables:

- `AlphaTest` → `alpha_test`
- `BetaTest` → `beta_test`
- `CoreTest` → `core_test`

The `alpha` and `beta` libraries both depend on `core`. This lets us inspect:

1. CTest command executable ↔ File API executable artifact mapping.
2. Compiler `.d` prerequisite syntax and translation-unit evidence.
3. Reverse target-impact propagation when a dependency changes.
4. Full-suite fallback after deliberately removing one required evidence file.

Do not treat this fixture as proof of support for arbitrary generators, wrappers, custom commands, or stale metadata.
