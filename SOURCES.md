# Primary Sources and Planning References

Access dates should be refreshed in the final repository. These links support planning; implementation must verify the exact versions encountered after kickoff.

## Hackathon

- [Zero Dependency official website](https://zerodepshack.com/) — tracks, zero-dependency definition, submission requirements, bonuses, scoring, rules, timeline, and FAQ.
- Organizer email from Maksim Muravev to Ayan Khan — runtime invocation prohibited; generated metadata permitted with disclosure and graceful degradation; stdin allowed; build tools permitted; C++ includes libc/POSIX; Single File scope; same-machine reproducibility; Package Killer interpretation; no code before kickoff. Preserve the original email separately.

## CMake File API

- [cmake-file-api(7)](https://cmake.org/cmake/help/latest/manual/cmake-file-api.7.html) — query/reply layout, index file, codemodel v2, configurations, target objects, sources, artifacts, dependencies, and versioning.
- [CMake File API codemodel v2 target object](https://cmake.org/cmake/help/latest/manual/cmake-file-api.7.html#codemodel-version-2-target-object) — target-specific fields and semantics.
- [CMake configure_file / File API general documentation index](https://cmake.org/cmake/help/latest/) — use official versioned docs for encountered toolchain.

Important planning fact: CMake writes `index-*.json` beneath `<build>/.cmake/api/v1/reply/`, and clients should read the index before referenced reply files. Target ids are opaque and must not be interpreted.

## CTest JSON

- [ctest(1)](https://cmake.org/cmake/help/latest/manual/ctest.1.html) — `--show-only=json-v1` and Show as JSON Object Model.
- [CTest Show as JSON Object Model](https://cmake.org/cmake/help/latest/manual/ctest.1.html#show-as-json-object-model) — `ctestInfo` kind/version and test structures.

Important planning fact: `--show-only` lists test information without executing tests; `json-v1` provides a structured format. TestImpact++ consumes a file created earlier and does not invoke CTest.

## Compiler dependency files

- [GCC Preprocessor Options](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html) — `-M`, `-MM`, `-MD`, `-MMD`, and `-MF` dependency generation.
- [Clang Command Line Argument Reference](https://clang.llvm.org/docs/ClangCommandLineReference.html) — verify Clang-compatible dependency flags if Clang is claimed.
- [GNU make manual: Rule Syntax / Splitting Long Lines / escaping](https://www.gnu.org/software/make/manual/html_node/Rule-Syntax.html) — relevant only to the subset of Make syntax actually emitted and supported; do not claim general Makefile parsing.

Important planning fact: GCC `-MD` can emit dependency files as a side effect of compilation, while `-MMD` excludes system headers. The chosen mode affects what completeness claim is defensible.

## C++ standard library

- [cppreference: filesystem library](https://en.cppreference.com/w/cpp/filesystem.html)
- [cppreference: variant](https://en.cppreference.com/w/cpp/utility/variant.html)
- [cppreference: optional](https://en.cppreference.com/w/cpp/utility/optional.html)
- [cppreference: string_view](https://en.cppreference.com/w/cpp/string/basic_string_view.html)
- [cppreference: algorithms](https://en.cppreference.com/w/cpp/algorithm.html)

cppreference is a practical secondary reference. Where standard conformance is disputed, consult the relevant ISO C++ working draft/toolchain documentation.

## Build reproducibility

- [reproducible-builds.org documentation](https://reproducible-builds.org/docs/) — sources of build nondeterminism and mitigation concepts.
- GCC/Clang/linker documentation for the exact selected flags — record exact versions during the event.

## Source-use policy

- Prefer primary official documentation for format semantics.
- Do not copy implementation code from parsers/libraries.
- Record any dev-only test framework and disclose it if used.
- Separate verified format facts from project policy decisions.
- Update source links if the actual event toolchain uses older format versions than latest documentation.

