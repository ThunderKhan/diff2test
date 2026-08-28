# Organizer Clarifications

## Status

These rulings were received by email from Maksim Muravev, CEO & Founder of Hackathon Raptors, after Ayan Khan asked eight eligibility questions. Preserve the original email and headers separately as evidence.

This file paraphrases the answer into engineering rules. Where exact wording matters, the relevant sentence is quoted briefly.

## Confirmed rulings

### 1. Runtime invocation of tools is prohibited

> “No. Invoking Git, CMake or CTest at runtime is shelling out to a separately installed tool…”

Consequences:

- diff2test must not call Git, CMake, CTest, compilers, Python, shells, or other installed tools.
- No `system`, `popen`, `exec`, process spawning, or indirect command launch is allowed.
- Help text and documentation must not imply automatic metadata generation.

### 2. Parsing pre-generated metadata is permitted

> “Yes. Parsing files those tools already produced is fine…”

Conditions:

- disclose these external data producers in `STDLIB.md`;
- degrade gracefully when input is absent;
- do not embed third-party code into the artifact;
- parse the files with code written during the event.

Permitted data inputs include:

- CMake File API replies;
- `compile_commands.json` if used;
- compiler `.d` files;
- CTest `--show-only=json-v1` output.

### 3. Piped changed paths are permitted

Reading changed paths from stdin is standard-library I/O. A user may externally pipe `git diff --name-only` into diff2test; diff2test itself must not launch Git.

### 4. Build tools are permitted

CMake may be used solely as build tooling. The compiler, build tool, and formatter do not count as runtime dependencies.

### 5. C++ eligibility includes libc and POSIX

The organizer confirmed that C and C++ entries may use libc and POSIX, not only ISO C++. The MVP should still prefer portable ISO C++ facilities where sufficient and disclose any POSIX-specific behavior.

### 6. Single File applies to implementation

One implementation source file is required: no `src/` tree and no implementation modules. Separate tests, documentation, fixtures, and build scripts are allowed.

### 7. Reproducibility scope

For the Reproducible Build bonus, two builds on the same machine with the same toolchain must produce byte-identical artifacts and published hashes. Cross-environment reproducibility is not required.

### 8. Package Killer eligibility

A submission may qualify by naming a real installable test-impact package, comparing features, and documenting substitutions in `STDLIB.md`. Higher package download counts increase weight. No Package Killer claim should be made until a specific package and honest feature comparison are established.

## Additional organizer guidance

1. Pick one bonus and execute it thoroughly.
2. Because C++ has no conventional dependency manifest, dependency proof carries extra weight. The organizer suggested dynamic-link inspection such as `ldd` output or build files showing no third-party retrieval.
3. No project code may be written before 28 August 2026 at 18:00 UTC. Planning, sketching, and studying standard-library documentation are permitted. A pre-existing project is ineligible.

## Locked project interpretation

| Topic | Project rule |
|---|---|
| Runtime subprocesses | Forbidden |
| Generated metadata | Allowed as disclosed input |
| Missing input | Must degrade safely and remain useful |
| Changed paths | File or stdin |
| Build system | CMake allowed after kickoff |
| Runtime language surface | C++ standard library; libc/POSIX permitted |
| Primary bonus | Single File |
| Implementation before kickoff | Forbidden |
| Planning documents before kickoff | Allowed |

## Pre-kickoff compliance checklist

- [ ] No `.cpp`, `.c`, header, module, script, or build file exists for the submitted project.
- [ ] No executable tests or reusable project fixtures exist.
- [ ] No repository is presented as if implementation has begun.
- [ ] Planning documents contain contracts and prose only.
- [ ] The organizer email is preserved.
- [ ] Kickoff time is recorded as 28 August 2026, 18:00 UTC (23:30 IST).

## Questions considered resolved

All eight emailed questions are resolved sufficiently to proceed. Any new behavior involving runtime execution, bundled external code, or an undisclosed data producer requires a fresh eligibility check.
