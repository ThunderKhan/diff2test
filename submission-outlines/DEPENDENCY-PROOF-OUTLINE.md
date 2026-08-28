# Dependency Proof Outline

## Claim

diff2test ships with zero third-party runtime dependencies and contains no vendored third-party source. It uses permitted C++/libc/POSIX runtime facilities and never launches separately installed tools.

CMake is intentionally treated as **permitted external build/input-generation tooling**, not as a runtime dependency of the shipped executable. The organizer explicitly confirmed that the compiler and build tool do not count against the runtime-dependency rule. The proof should make that distinction visible rather than leaving judges to infer it.

## Environment

Record exact:

- OS/distribution/version;
- architecture;
- compiler/version;
- CMake/build tool/version;
- build type and flags;
- commit hash.

## Clean build evidence

Include commands and captured output for:

1. clean checkout;
2. empty/new build directory;
3. one-command build;
4. produced artifact.

Verify build files contain no dependency download, package lookup for third-party runtime libraries, submodules, vendored paths, FetchContent, CPM, vcpkg, Conan, or network step.

If the one-command build uses CMake, explain immediately after the command that CMake is being used only as the permitted build tool. Do not present the existence of `CMakeLists.txt` as evidence of a runtime dependency.

## Build-tool/runtime boundary proof

Document the boundary explicitly:

- `cmake` may configure/build the project externally;
- `ctest`/CMake may generate fixture or analysis metadata externally;
- none of those executables is launched by `diff2test`;
- the produced `diff2test` binary has no CMake runtime linkage/package dependency;
- when already-generated inputs are supplied, runtime analysis does not require a CMake process;
- missing metadata causes conservative fallback, not subprocess execution.

Include the organizer ruling or a short paraphrase/reference in the final proof. The precise zero-dependency claim is about the **shipped runtime artifact**, not about pretending no build tool exists.

## Repository inventory

Show concise tree and explain:

- one implementation source;
- separate original tests/fixtures/docs;
- no `vendor`, third-party source, binary blobs, generated library code, or dependency lockfile carrying runtime packages.

## Dynamic dependency inspection

On Linux, capture:

```bash
ldd <actual-artifact>
```

Explain every reported system/toolchain runtime library. Do not claim “statically zero dependencies” if the binary dynamically links permitted libc/libstdc++/loader. The claim is zero **third-party runtime dependencies** under event rules.

Specifically verify that no CMake library appears as a dynamic dependency of the artifact.

Optionally supplement with:

- `file`;
- `readelf -d`;
- linker map or verbose link command;
- `nm` as appropriate.

Use only tools available for proof; none are invoked by the program.

## No subprocess proof

Record a source audit/search for process-spawning APIs and shell invocation. Check terms such as:

- `system`;
- `popen`/`pclose`;
- `fork`/`exec`/`posix_spawn`;
- Windows process APIs if any cross-platform branch exists;
- shell command strings.

Explain false-positive matches instead of hiding them.

The audit should explicitly support the claim that `diff2test` cannot invoke CMake, CTest, Git, compilers, shells, or other installed tools at runtime.

## No network/service proof

State that runtime has no network component or service configuration. A network-disabled run/build smoke test may supplement but not replace source/build inspection.

## Metadata boundary

List inputs produced externally:

- changed path list;
- File API reply;
- `.d` files;
- CTest JSON.

State that they are parsed as untrusted data and absence triggers safe degradation. diff2test does not execute their producers.

Clarify that CMake/CTest-generated files are **data inputs**, not linked libraries or hidden runtime tool calls.

## Reproduction commands

Give a judge the shortest commands to repeat build, dynamic dependency inspection, no-subprocess source audit, and smoke analysis.

Keep build-time commands and runtime-analysis commands visibly separate so the CMake boundary is obvious.

## Result

Summarize pass/fail and date. Attach raw output in a separate `deps-proof.txt` if long.

## Checklist

- [ ] Exact commit/toolchain recorded
- [ ] Clean build output included
- [ ] CMake explicitly identified as permitted build/input-generation tooling, not runtime dependency
- [ ] No third-party lookup/download
- [ ] Repository tree inspected
- [ ] Dynamic dependencies explained
- [ ] No CMake runtime linkage
- [ ] No subprocess APIs
- [ ] No service/network requirement
- [ ] Metadata distinction explained
- [ ] Commands reproducible by judge
