# Reproducible Build Outline

Attempt only after all P0 functionality, tests, documentation, dependency proof, and demo readiness are green.

## Bonus requirement

The organizer confirmed that two builds on the same machine with the same toolchain must produce byte-identical artifacts with both hashes published. Cross-environment reproducibility is not required.

## Fixed environment

Record:

- machine/OS/architecture;
- compiler and linker versions;
- CMake/build tool version;
- locale/timezone/environment variables relevant to build;
- source commit;
- identical build command;
- build type/flags.

## Procedure

1. Start from the same clean source commit.
2. Create two distinct empty build directories.
3. Apply the same controlled environment.
4. Build each using the same command.
5. Copy/identify the two artifacts without modifying bytes.
6. Hash both with the same algorithm, preferably SHA-256.
7. Compare bytes directly.
8. Publish commands and outputs.

## Potential nondeterminism checklist

- `__DATE__`, `__TIME__`, `__TIMESTAMP__`;
- absolute build/source paths in debug info;
- linker build IDs;
- timestamps in archive members;
- unordered generated input/source order;
- locale-dependent output during generation;
- random seeds;
- embedded Git state/time;
- parallel link ordering;
- profile/coverage instrumentation;
- debug symbols.

Choose a release build and flags appropriate to the toolchain. Do not strip useful product information merely for a badge without documenting the tradeoff.

## Evidence table

| Item | Build A | Build B |
|---|---|---|
| Source commit | `<fill>` | `<same>` |
| Build directory | `<fill>` | `<fill>` |
| Command | `<fill>` | `<same>` |
| Artifact size | `<fill>` | `<fill>` |
| SHA-256 | `<fill>` | `<fill>` |
| Byte comparison | `<fill>` | `<fill>` |

## Outcome

If identical, claim the bonus with raw logs. If not identical after a strict two-hour cap, preserve the attempt as engineering notes but do not claim success or distract from submission quality.

## Checklist

- [ ] P0 complete before attempt
- [ ] Same commit/machine/toolchain
- [ ] Two clean directories
- [ ] Same controlled environment/command
- [ ] Hashes published
- [ ] Byte comparison published
- [ ] No misleading cross-platform claim

