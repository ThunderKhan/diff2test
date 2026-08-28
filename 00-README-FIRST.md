# TestImpact++ Pre-Hackathon Planning Pack

**Owner:** Ayan Khan  
**Team:** `std::zero`  
**Track:** A — Developer Tools & CLI  
**Planning snapshot:** 12 August 2026  
**Coding window:** 28 August 2026, 18:00 UTC to 31 August 2026, 18:00 UTC

## Purpose

This folder is a planning system for building TestImpact++ during the official 72-hour Zero Dependency Hackathon. It contains product decisions, input and CLI contracts, safety rules, technical risks, tests to write after kickoff, and a timed runbook.

It intentionally contains **no project implementation**, executable test, reusable fixture, build script, or C++ source. The official website and the organizer's written response prohibit project code before kickoff.

## One-sentence product definition

TestImpact++ is a zero-runtime-dependency C++20 CLI that consumes changed paths and pre-generated CMake/compiler/CTest metadata, selects only the tests whose impact can be justified by that evidence, explains every selection, and requests the full suite whenever the evidence is incomplete.

## Locked decisions

1. The program will never launch Git, CMake, CTest, a compiler, a shell, Python, or another executable at runtime.
2. Changed paths arrive through a file or standard input; an external shell may pipe Git output into the program.
3. CMake File API replies, compiler `.d` files, and CTest `json-v1` output are data inputs, not programs invoked by TestImpact++.
4. Missing, malformed, stale, ambiguous, or unsupported evidence expands the selection to all known tests or emits `FULL_SUITE_REQUIRED` when no catalogue is available.
5. The primary bonus target is **Single File**: one readable `testimpact.cpp`, with tests, docs, fixtures, and build files separate.
6. Linux with GCC-compatible dependency files is the MVP platform. Broader platform support is a stretch goal.
7. Safety is asymmetric: selecting too many tests is acceptable; silently skipping a possibly affected test is not.

## Reading order

1. `PROBLEM-BRIEF.md`
2. `ORGANIZER-CLARIFICATIONS.md`
3. `FEASIBILITY-REVIEW.md`
4. `PRD.md`
5. `MVP.md`
6. `INPUT-SPEC.md`
7. `CLI-CONTRACT.md`
8. `SAFETY-CONTRACT.md`
9. `ARCHITECTURE.md`
10. `TEST-PLAN.md`
11. `DEMO-CONTRACT.md`
12. `TASKS.md`
13. `KICKOFF-RUNBOOK.md`
14. `CLAUDE.md`
15. `submission-outlines/`
16. `SOURCES.md`

## Authority order

If two documents conflict, use this order:

1. Official hackathon rules and written organizer clarifications
2. `SAFETY-CONTRACT.md`
3. `MVP.md`
4. `INPUT-SPEC.md` and `CLI-CONTRACT.md`
5. `ARCHITECTURE.md`
6. `PRD.md`
7. Remaining planning documents

Record any deliberate contract change in the hackathon repository before implementing it.

## Pre-kickoff rule

Before 28 August 2026 at 18:00 UTC, use this pack only for reading, reviewing, editing prose, studying official documentation, and preparing prompts. Do not turn its pseudocode-level descriptions into project code, tests, fixtures, build scripts, or repository scaffolding.

## Definition of a successful submission

The submission succeeds if a judge can quickly verify that it:

- builds in one command;
- contains one readable implementation source file;
- has zero third-party runtime dependencies;
- never shells out to installed tools;
- correctly selects a smaller test set for at least one realistic case;
- explains why each selected test is affected;
- safely requests the full suite when evidence becomes incomplete;
- documents limitations and stdlib substitutions honestly;
- can be demonstrated from a clean checkout within five minutes.

