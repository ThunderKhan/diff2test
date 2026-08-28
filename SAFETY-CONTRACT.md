# Safety Contract — diff2test

## 1. Core invariant

diff2test may output a proper subset of registered tests only when it has positive, complete, supported evidence that every omitted registered test is unaffected under the model it claims to support.

When completeness cannot be established, it must not guess.

## 2. Outcome states

### `SUBSET_SELECTED`

All required evidence is valid and complete under the supported model. Output is a justified subset of the known catalogue.

### `FULL_SUITE_SELECTED`

CTest catalogue is readable, but another evidence source is absent, invalid, stale, unsupported, or ambiguous. Every known test is emitted.

### `FULL_SUITE_REQUIRED`

The registered test catalogue cannot be trusted or enumerated. No test list is invented. The caller is told to run the complete suite through its existing workflow.

### `USAGE_ERROR`

The caller did not provide a meaningful analysis request.

### `INTERNAL_ERROR`

An invariant or unexpected exception failed. Never emit a partial subset after this state.

## 3. Decision order

1. Validate CLI and changed-path source.
2. Parse the CTest catalogue early enough to know whether full-known selection is possible.
3. Validate roots and metadata versions.
4. Load and completeness-check File API/codemodel data.
5. Map every registered test under the supported exact-artifact rules.
6. Load and completeness-check the explicit dependency-file list.
7. Reject dependency evidence whose project prerequisites are detectably newer than the corresponding `.d` file.
8. Normalize/classify every changed path.
9. Build the reverse target graph and traverse impact.
10. Only then commit to subset output.

No tentative subset names are emitted before the final safety decision.

## 4. Safety matrix

| Condition | Required outcome | Reason |
|---|---|---|
| CTest file absent/unreadable | `FULL_SUITE_REQUIRED` | cannot enumerate suite |
| CTest JSON malformed/unsupported major | `FULL_SUITE_REQUIRED` | catalogue untrusted |
| duplicate/empty test name | `FULL_SUITE_REQUIRED` | catalogue ambiguous |
| empty command array | `FULL_SUITE_SELECTED` if catalogue itself remains trustworthy | mapping incomplete |
| wrapper/unmapped/ambiguous test command | `FULL_SUITE_SELECTED` | omitted wrapper test cannot be proven unaffected |
| File API reply absent | `FULL_SUITE_SELECTED` | target/source mapping unavailable |
| ambiguous reply index | `FULL_SUITE_SELECTED` | configuration snapshot unclear |
| codemodel unsupported/malformed | `FULL_SUITE_SELECTED` | target graph untrusted |
| build/project root mismatch | `FULL_SUITE_SELECTED` | paths may refer to a different tree |
| multiple configs without explicit selection | `FULL_SUITE_SELECTED` | artifact identity ambiguous |
| target object missing | `FULL_SUITE_SELECTED` | graph incomplete |
| generated source encountered | `FULL_SUITE_SELECTED` | generated/custom-command chain unsupported |
| dependency list absent/unreadable | `FULL_SUITE_SELECTED` | include evidence unavailable |
| listed `.d` absent/malformed | `FULL_SUITE_SELECTED` | include evidence incomplete |
| duplicate `.d` / translation-unit evidence | `FULL_SUITE_SELECTED` | completeness is ambiguous |
| `.d` target cannot map uniquely | `FULL_SUITE_SELECTED` | translation-unit ownership unclear |
| project prerequisite timestamp cannot be inspected | `FULL_SUITE_SELECTED` | freshness audit incomplete |
| project prerequisite newer than `.d` file | `FULL_SUITE_SELECTED` | detectable stale dependency evidence |
| required translation unit has no `.d` evidence | `FULL_SUITE_SELECTED` | dependency coverage incomplete |
| changed source/header known | continue | evidence node exists |
| changed path unknown | `FULL_SUITE_SELECTED` | it may affect generation/configuration |
| changed `CMakeLists.txt`/`.cmake` | `FULL_SUITE_SELECTED` | build graph may change |
| target dependency references unknown target | `FULL_SUITE_SELECTED` | propagation graph incomplete |
| graph cycle | traverse with visited set | cycles alone are not corruption |
| path escapes declared roots | no subset | unsafe normalization |
| configured input limit exceeded | full known suite or required | cannot safely finish analysis |
| unexpected exception | `INTERNAL_ERROR` | state unknown |

## 5. Unknown-path policy

MVP uses an allowlist mindset. A changed path is known if it appears as project dependency evidence or as an explicitly recognized build-configuration path that causes fallback.

Do not infer safety from extensions such as `.md`, `.txt`, or image files. A documentation file can still be consumed by a generator or test. Unknown project paths therefore widen to the full known suite rather than being silently ignored.

## 6. Deleted and renamed paths

Deleted files may not appear in current metadata. Therefore any changed path that cannot be mapped in current evidence triggers full selection. For renames, callers should supply both old and new paths; if the old path is unknown, fallback remains correct.

## 7. Completeness model

An evidence source is not “complete” merely because some files were found.

### CTest completeness

Requires a valid catalogue and a unique supported executable-artifact mapping for every registered test.

### Codemodel completeness

Requires every referenced in-scope target object to be present and supported under exactly one selected configuration, with declared source/build roots matching the caller's roots.

### Dependency completeness

The MVP uses an explicit `--dep-list`. For every supported compiled `.cpp` translation unit, identified as a `(CMake target, source)` pair, exactly one trusted dependency-file mapping must exist before subset output is allowed.

This is deliberately global for the in-scope codemodel. It may over-fallback, but it avoids treating absence of evidence as evidence of absence.

### Current dependency-target layout boundary

For the Unix Makefiles-style layout validated by the controlled fixture, a dependency rule target is associated with a CMake target through the observed form:

```text
CMakeFiles/<target-name>.dir/...
```

A rule that does not map uniquely under this supported policy causes full-suite fallback. This is not a claim that every CMake generator uses the same layout.

## 8. Freshness model

The implementation performs a **detectable-staleness audit** for project prerequisites in every trusted `.d` file:

1. read the `.d` file modification timestamp;
2. read the modification timestamp of each prerequisite that resolves inside the declared project root;
3. if a project prerequisite is newer than the `.d` file, reject the evidence and emit the full known suite;
4. if either timestamp cannot be inspected, reject subset selection rather than assuming freshness.

This check does **not** prove absolute correspondence between source state and metadata. Equal/older timestamps can still be misleading in unusual or adversarial environments, clock behavior can be imperfect, and the tool does not cryptographically bind metadata to source contents.

Users must therefore generate File API, compiler dependency, and CTest metadata in the same normal build workflow immediately before analysis. Passing the timestamp audit means **no detectable staleness under the supported check**, not “cryptographically fresh metadata.”

## 9. Explanation requirements

Every subset-selected test receives a concrete evidence chain containing:

```text
changed path
  → dependency file
  → affected translation unit
  → owning target
  → zero or more dependent targets
  → registered CTest test
```

The current implementation prints concrete project-relative changed paths, build-relative dependency-file paths, translation-unit paths, target names, and test names. This is intended to make a judge or CI engineer able to audit why the test was selected.

Full-suite fallback emits the concrete reason that prevented safe omission, such as missing dependency coverage, stale evidence, root mismatch, unsupported mapping, or an unknown changed path.

## 10. Exit and output rule

Safety status is decided before stdout is finalized. Names mode emits:

- subset names for `SUBSET_SELECTED`;
- every catalogue name for `FULL_SUITE_SELECTED`;
- no names for `FULL_SUITE_REQUIRED`.

Fallback/required reasons are written to stderr. The deliberate non-zero safety statuses force CI integrations to acknowledge fallback instead of silently treating it as precise subset success.

## 11. Forbidden shortcuts

- treating a missing `.d` file as “no dependencies”;
- accepting a detectably stale `.d` file;
- basename-only executable matching;
- selecting tests by `*_test.cpp` filename conventions;
- silently ignoring unknown required JSON structure;
- continuing through unsupported major versions;
- assuming unknown changed extensions are harmless;
- outputting a partial result alongside an internal error;
- labeling fallback as precise selection;
- launching Git, CMake, CTest, a compiler, a shell, Python, or another external executable to fill missing evidence.

## 12. Safety review gate

The integration mutation suite deliberately removes or corrupts evidence and asserts that the result loses `SUBSET_SELECTED`. It currently covers missing/malformed/duplicate dependency evidence, stale evidence, unknown and build-configuration changes, CTest catalogue failures, wrapper-style test commands, malformed/ambiguous CMake metadata, root mismatch, and unknown target edges.

The public CI additionally generates metadata from the controlled CMake fixture using external developer/build tooling, verifies a real narrow subset, removes required dependency evidence, and verifies the full-suite fallback. Any mutation that still produces subset success is a release blocker.
