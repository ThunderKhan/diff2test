# Metadata Spike Results

Date: 28 August 2026, after official kickoff.

This document records the first feasibility spike required by `TASKS.md` and `KICKOFF-RUNBOOK.md`. It is evidence about one controlled Linux/CMake/GNU-Make-style environment, not a claim of universal generator behavior.

## Environment used for the spike

- Linux container
- GCC 14.2.0
- CMake 3.31.6
- Unix Makefiles generator

## File API query placement

A source-tree `.cmake/api/v1/query/codemodel-v2` marker does **not** request File API replies. The query must be created under the build tree before configure:

```text
<build>/.cmake/api/v1/query/codemodel-v2
```

The fixture documentation now uses that layout.

## CTest command ↔ CMake artifact mapping

Observed CTest command tokens were absolute paths:

```text
AlphaTest -> <build>/alpha_test
BetaTest  -> <build>/beta_test
CoreTest  -> <build>/core_test
```

Observed File API executable artifacts were build-root-relative paths:

```text
alpha_test -> alpha_test
beta_test  -> beta_test
core_test  -> core_test
```

MVP mapping conclusion: resolve a reported File API artifact relative to the verified build root, normalize lexically, then require an exact unique match with the normalized CTest command executable. Basename-only matching remains forbidden.

## Target dependency direction

Observed File API relationships encode `target depends on dependency`:

```text
alpha      depends on core
beta       depends on core
alpha_test depends on alpha and core
beta_test  depends on beta and core
core_test  depends on core
```

Impact analysis therefore needs a reverse adjacency when asking what becomes affected if `core` changes.

## Compiler dependency files

Observed object dependency files include paths such as:

```text
CMakeFiles/alpha.dir/src/alpha.cpp.o.d
CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d
```

A representative rule used a build-relative object target and absolute prerequisites, with backslash-newline continuation:

```text
CMakeFiles/alpha.dir/src/alpha.cpp.o: <source>/src/alpha.cpp \
 <system-header> <source>/include/alpha.hpp \
 <source>/include/common.hpp
```

The build tree also contained non-compilation `.d` files such as `link.d`. Therefore recursive discovery cannot assume that every `.d` file is compiler prerequisite evidence. The safest MVP path is to prefer an explicit dependency-file list unless discovery is implemented with a proven mapping policy.

## Result of the gate

The core concept remains feasible for the controlled MVP:

- direct artifact/test mapping is defensible;
- dependency direction is understood;
- real `.d` syntax matches the planned parser class;
- no runtime subprocess is needed.

The next implementation work should keep exact mapping and conservative fallback as release blockers.
