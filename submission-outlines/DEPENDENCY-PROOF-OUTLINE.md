# Dependency Proof Outline

## Claim

diff2test ships with zero third-party runtime dependencies and contains no vendored third-party source. It uses permitted C++/libc/POSIX runtime facilities and never launches separately installed tools.

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

## No network/service proof

State that runtime has no network component or service configuration. A network-disabled run/build smoke test may supplement but not replace source/build inspection.

## Metadata boundary

List inputs produced externally:

- changed path list;
- File API reply;
- `.d` files;
- CTest JSON.

State that they are parsed as untrusted data and absence triggers safe degradation. diff2test does not execute their producers.

## Reproduction commands

Give a judge the shortest commands to repeat build, dynamic dependency inspection, no-subprocess source audit, and smoke analysis.

## Result

Summarize pass/fail and date. Attach raw output in a separate `deps-proof.txt` if long.

## Checklist

- [ ] Exact commit/toolchain recorded
- [ ] Clean build output included
- [ ] No third-party lookup/download
- [ ] Repository tree inspected
- [ ] Dynamic dependencies explained
- [ ] No subprocess APIs
- [ ] No service/network requirement
- [ ] Metadata distinction explained
- [ ] Commands reproducible by judge
