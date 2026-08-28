# 72-Hour Kickoff Runbook

## Event clock

- Kickoff: **28 August 2026, 18:00 UTC / 23:30 IST**
- Code freeze: **31 August 2026, 18:00 UTC / 23:30 IST**
- Solo team: `std::zero`

Protect sleep. A tired parser author creates exactly the edge cases this project promises to handle. Suggested sleep blocks are included and may shift, but do not plan 72 continuous waking hours.

## Before kickoff (planning only)

- read this pack and official docs;
- verify global compiler/CMake/Git/editor installation;
- preserve organizer email;
- prepare a timer and submission checklist;
- prepare post-kickoff Claude prompts as prose;
- do not create project code, fixtures, tests, build scripts, or official repo content early.

## H+0:00 to H+0:15 — Start clean

1. Confirm official start announcement/time.
2. Create project directory and Git repository.
3. Record toolchain versions.
4. Create first in-window commit.
5. Start a work log with UTC timestamps.

## H+0:15 to H+2:00 — Fatal-risk spikes

Perform T0.2–T0.5 in order.

Decision gates:

- If direct CTest-command ↔ target-artifact mapping works, continue.
- If it only differs by a clear build-root-relative normalization, document and continue.
- If wrappers are required in the controlled project, make direct executables the supported MVP and wrappers a fallback.
- If `.d` discovery is unreliable, accept an explicit list rather than writing generator-specific guessing.
- If source/object mapping cannot be evidenced in two hours, pivot to an explicit metadata manifest only if it remains useful and rules-compliant; otherwise pivot project.

Commit: `spike: validate metadata mapping assumptions`.

## H+2 to H+6 — Foundation

- one-command build;
- single source file shell written now;
- test harness;
- diagnostics/statuses;
- CLI parser/help.

Do not begin polished output or graph work.

Commit checkpoints:

- `build: add one-command C++20 build`
- `feat: define CLI and status contract`

## H+6 to H+12 — JSON core

- implement strict parser;
- immediately write valid/malformed tests;
- include limits and positional errors;
- implement typed accessors.

At H+10, if core JSON is not stable:

- remove any planned JSON output serialization;
- avoid optimization/zero-copy work;
- support only one owning tree;
- ask AI for review/test generation, not a wholesale replacement.

Commit: `feat: parse JSON with positional diagnostics`.

## H+12 to H+16 — Dependency and path parsers

- dependency state machine;
- escaping/continuation tests;
- root-aware path normalization;
- containment tests.

Commit: `feat: parse dependency evidence and normalize paths`.

## H+16 to H+18 — Sleep/food checkpoint

Save work, push, write the next exact task in the log, and sleep 5–6 hours if following a normal schedule. If clock placement differs because kickoff is late IST, sleep after completing a coherent parser boundary rather than accumulating fragile late-night code.

## H+18 to H+28 — Metadata loaders

- CTest catalogue first;
- File API index/codemodel/configuration;
- target objects;
- `.d` discovery/mapping;
- exact test mapping.

Run loader tests after each layer. Never combine all loaders before testing.

At H+22, if behind:

- require explicit configuration;
- require explicit CMake index when multiple exist;
- use `--dep-list`;
- exclude target propagation temporarily;
- keep direct test-target support.

Commits by component.

## H+28 to H+36 — First vertical slice

- graph types/reverse indexes;
- direct source/header impact;
- target/test connection;
- one explanation path;
- human output.

Milestone at H+34: one supported changed header must select one correct test end-to-end.

If not:

- stop new JSON/CLI features;
- use the controlled fixture only;
- remove optional target propagation;
- focus on direct owning target and mapped test.

Commit: `feat: select and explain affected tests end to end`.

## H+36 to H+46 — Safety before breadth

- central safety ledger;
- all-known selection;
- full-required behavior;
- missing/malformed mutation cases;
- wrapper mapping failure;
- unknown changed path;
- deterministic output.

Release blocker: no mutation may produce subset success.

Commit: `feat: enforce conservative full-suite fallback`.

## H+46 to H+52 — Integration and hardening

- finish three-test demo fixture;
- chain/diamond/cycle tests if target propagation retained;
- resource limits;
- warning/sanitizer sweep;
- repeated deterministic runs.

At H+52: feature freeze. Tag or note `mvp-freeze`.

## H+52 to H+58 — Documentation

Write from the real implementation:

- README usage/limits;
- STDLIB substitutions;
- dependency proof steps;
- architecture summary;
- exact demo commands.

Do not copy aspirations from the PRD as claims. Every support claim requires a passing case.

## H+58 to H+62 — Clean-room verification

- new clean directory/clone;
- one-command build;
- all tests;
- selective scenarios;
- safety scenarios;
- dependency inspection;
- check repository for secrets, personal paths, pre-kickoff code, binaries, and unrelated files.

## H+62 to H+66 — Bonus gate and polish

Only if every P0 is green:

- attempt same-toolchain reproducible build for at most two hours;
- polish help/output;
- verify 10 STDLIB substitutions;
- do not chase Package Killer without credible package evidence.

If any P0 is red, skip bonuses.

## H+66 to H+69 — Demo

- rehearse twice;
- record;
- inspect audio/video beginning, middle, end;
- upload with time buffer;
- keep original backup.

## H+69 to H+71 — Submission rehearsal

- open repository as a judge;
- follow README literally;
- verify public visibility/license;
- verify links and video access;
- fill submission form without submitting if preview is available;
- capture final commit hash.

## H+71 to H+72 — Buffer and submit

- no feature work;
- only critical correctness/docs fixes;
- submit at least 20–30 minutes before deadline if platform permits;
- preserve confirmation screenshot/email.

## Claude Code prompt sequence after kickoff

### Prompt A — Spike reviewer

Ask Claude to read the planning pack and inspect newly generated metadata, then report exact mappings and contradictions. Permit reads only initially; do not ask it to build the whole project.

### Prompt B — Task implementer

Give one bounded task from `TASKS.md`, its acceptance cases, relevant contract sections, and permission to edit only after restating the plan. Require tests and a summary of changed files.

### Prompt C — Safety adversary

After vertical slice, ask Claude to find any path by which missing evidence still reaches subset success. Require references to the safety matrix and new failing tests before fixes.

### Prompt D — Single-file reviewer

Ask for readability review: section dependencies, functions too large, duplicated parsing logic, implicit ownership, nondeterministic iteration, and exception leaks. Do not split implementation into multiple files.

### Prompt E — Submission auditor

Ask Claude to compare README/STDLIB/dependency proof against actual code/build behavior and flag unsupported claims, hidden tool execution, third-party lookups, and demo commands that fail from clean checkout.

## Commit rhythm

Commit coherent, verified states roughly every 1–3 hours. Do not create artificial commits to simulate activity. Examples:

- spike validation
- build/CLI foundation
- JSON parser
- dependency/path parser
- CTest loader
- File API loader
- mapping/graph
- safety fallback
- integration tests
- docs/proof

Push after every major milestone and before sleep.

## Emergency scope ladder

### Level 1

Cut colors, JSON output, verbosity extras, compile database.

### Level 2

Require explicit index/configuration/dep list; cut multi-config convenience and auto-discovery.

### Level 3

Cut target dependency propagation; support direct source/header → owning test executable target only.

### Level 4

Support only the controlled documented CMake/GCC/CTest shape, but keep strict validation and fallback for everything else.

### Never cut

- no-subprocess rule;
- honest docs;
- correct JSON and `.d` handling for supported inputs;
- complete-catalogue safety;
- all/full-required fallback;
- dependency proof;
- demo of both success and safe failure.

## Personal operating checklist

- eat before kickoff and at planned intervals;
- keep water nearby;
- use focused 50/10 or 75/15 blocks;
- keep browser tabs limited to official docs;
- log decisions immediately;
- after two failed attempts, reduce/reframe task before trying a third;
- sleep enough to explain every line to judges.

