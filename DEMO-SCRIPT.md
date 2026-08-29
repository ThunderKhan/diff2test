# diff2test — Five-Minute Demo Script

This script freezes the commands and narrative for the final hackathon recording. The commands below mirror scenarios exercised in public CI.

Target runtime: **4:30–4:50**.

## 0:00–0:20 — Hook

Say:

> Running every C++ test is safe but slow. Guessing which tests to skip is fast but dangerous. diff2test reads evidence the build already produced, selects only what it can justify, and asks for the full suite the moment that evidence breaks.

Show the repository root briefly and point to `diff2test.cpp`.

## 0:20–0:45 — Zero-dependency boundary

Show:

```bash
find . -maxdepth 2 -type f | sort | head -40
```

Point out:

- one runtime implementation source: `diff2test.cpp`;
- tests/fixture/docs are separate;
- no vendored third-party runtime source;
- CMake is build/input-generation tooling, not something the runtime launches.

Say:

> diff2test never runs Git, CMake, or CTest; it consumes changed paths and metadata those tools already produced.

## 0:45–1:15 — Clean build

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
./build/diff2test --version
```

## 1:15–1:35 — Runtime dependency proof

```bash
ldd build/diff2test
grep -nE '\b(system|popen|pclose|fork|exec[lvpe]*|posix_spawn)\s*\(' diff2test.cpp || true
```

Explain that linkage is limited to system/toolchain runtime components and the source audit finds no process-spawn path.

## 1:35–2:05 — Generate fixture metadata externally

```bash
rm -rf fixture/build
cmake -E make_directory fixture/build/.cmake/api/v1/query
cmake -E touch fixture/build/.cmake/api/v1/query/codemodel-v2
cmake -S fixture -B fixture/build -DCMAKE_BUILD_TYPE=Debug
cmake --build fixture/build
ctest --test-dir fixture/build --show-only=json-v1 > fixture/build/ctest-info.json
find fixture/build -type f -name '*.o.d' -printf '%P\n' | sort > fixture/build/deps.txt
```

Say:

> These are external workflow commands. diff2test does not execute them; it only reads the resulting files.

## 2:05–2:45 — Shorthand narrow selection + explanation

The fixture uses `fixture/build`, so the project root alone is enough:

```bash
printf 'include/alpha.hpp\n' | ./build/diff2test analyze fixture --explain
```

Point out:

- `STATUS: SUBSET_SELECTED`;
- only `AlphaTest` is selected;
- explanation contains changed path, `.d` evidence, translation unit, target chain, and registered test.

Then show the real-world composition form:

```bash
git diff --name-only HEAD~1 | ./build/diff2test analyze .
```

Say explicitly:

> The shell launches Git and creates the pipe. diff2test only reads stdin and existing metadata.

## 2:45–3:10 — Shared-header multi-test selection

```bash
printf 'include/features_shared.hpp\n' | ./build/diff2test analyze fixture --format names
```

Expected:

```text
AlphaTest
BetaTest
```

Say:

> The same metadata graph expands naturally when a header is shared across two feature paths.

## 3:10–3:55 — Remove evidence: safe fallback

Create an incomplete dependency list:

```bash
grep -v 'CMakeFiles/alpha.dir/' fixture/build/deps.txt > fixture/build/deps-incomplete.txt
```

Run with only the exceptional path overridden:

```bash
set +e
printf 'include/alpha.hpp\n' | ./build/diff2test analyze fixture \
  --dep-list fixture/build/deps-incomplete.txt \
  --format names
printf 'exit=%s\n' "$?"
set -e
```

Expected test names:

```text
AlphaTest
BetaTest
CoreTest
```

Expected exit:

```text
10
```

Say:

> Missing evidence is never interpreted as no dependency. The catalogue is known, so diff2test emits every known test and returns a visible fallback status.

## 3:55–4:20 — Missing catalogue

```bash
set +e
printf 'include/alpha.hpp\n' | ./build/diff2test analyze fixture \
  --ctest-info fixture/build/missing-ctest.json \
  --format names
printf 'exit=%s\n' "$?"
set -e
```

Expected:

- no invented test names;
- clear `CTest catalogue not found at ...` diagnostic;
- exit `11` (`FULL_SUITE_REQUIRED`).

## 4:20–4:50 — Close

Show `STDLIB.md` and `DEPENDENCY-PROOF.md`.

Say:

> The convenience is just deterministic file conventions and Unix stdin composition. The safety engine is unchanged: complete evidence gives a smaller suite; uncertainty gives everything, or explicitly asks the caller to run everything when the catalogue itself cannot be trusted.

## Pre-recording checklist

- [ ] clone/fetch latest `main`
- [ ] close notifications and unrelated terminals
- [ ] terminal font 18+ and readable at 1080p
- [ ] run the entire script once without recording
- [ ] run it a second time without recording
- [ ] confirm recording is under five minutes
- [ ] confirm repository/terminal contains no unrelated personal data
- [ ] keep original local recording until submission is accepted
- [ ] paste the final public video URL into the hackathon submission and README only after upload succeeds

## Evidence already automated

Public CI exercises clean checkout/build, shorthand-vs-fully-explicit equivalence, default stdin input, explicit overrides, narrow selection, shared-header selection, missing-evidence fallback, missing-catalogue behavior, deterministic output, dynamic-link inspection, subprocess audit, sanitizers, and same-toolchain reproducible Release builds.

The only parts that cannot be automated by the repository are the human narration, screen recording, upload, and final video-link insertion.
