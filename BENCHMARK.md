# Performance Snapshot

This document records a small, reproducible performance measurement for `diff2test`. It is intentionally a snapshot, not a cross-platform performance claim.

## Environment

Measured on GitHub Actions using:

- runner: `ubuntu-24.04`
- OS image: Ubuntu 24.04.4 LTS
- architecture: x86_64
- compiler: GCC 13.3.0
- CMake: 3.31.6
- Python: 3.12.3, used only by the external timing harness
- `diff2test`: Release build from commit `19a9df936d753f0e764c540fea29dc381d789dd0`

Python is not a runtime dependency of `diff2test`; it was used only to invoke the already-built executable repeatedly and measure wall-clock time.

## Scenario

The benchmark uses the repository's real CMake/CTest fixture and complete generated metadata.

Changed path:

```text
include/features_shared.hpp
```

Expected result on every invocation:

```text
AlphaTest
BetaTest
```

Each measured sample launches the CLI as a separate process and therefore includes normal process-startup overhead, input-file parsing, graph construction, impact analysis, and output generation.

## Method

- Release build with `-DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF`
- fixture metadata generated externally with CMake, the compiler, and CTest
- 20 warm-up invocations
- 200 measured invocations
- wall-clock timing via Python `time.perf_counter_ns()`
- every invocation required exit code `0` and exact expected stdout

## Results

| Metric | Time |
|---|---:|
| Median | **2.106 ms** |
| p95 | **2.209 ms** |
| Minimum | 2.036 ms |
| Maximum | 2.922 ms |

These numbers describe this specific small fixture on one hosted runner. They should not be extrapolated to large projects without separate measurements because runtime scales with the amount of CMake, CTest, and compiler dependency evidence that must be parsed and traversed.

## Existing scale-oriented verification

Performance is not used as a correctness substitute. The normal test suite separately includes a dependency-parser stress case with **10,000 prerequisites**, while CI verifies real selective analysis, deterministic output, conservative fallback behavior, sanitizers, and reproducible Release builds.

## Reproducing the measurement

The measurement was produced in a temporary GitHub Actions benchmark run from the same source commit. The benchmark harness itself is deliberately not part of the shipped runtime or permanent CI surface.
