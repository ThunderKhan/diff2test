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
2. Attempt to parse CTest catalogue early enough to know whether full known selection is possible.
3. Validate roots and metadata versions.
4. Load and completeness-check File API/codemodel.
5. Load and completeness-check dependency files.
6. Map every registered test under MVP mapping rules.
7. Normalize/classify every changed path.
8. Build graph and traverse impact.
9. Audit all safety flags.
10. Only then commit to subset output.

No streaming of tentative subset names to stdout before the safety audit completes.

## 4. Safety matrix

| Condition | Required outcome | Reason |
|---|---|---|
| CTest file absent/unreadable | `FULL_SUITE_REQUIRED` | cannot enumerate suite |
| CTest JSON malformed/unsupported major | `FULL_SUITE_REQUIRED` | catalogue untrusted |
| duplicate/empty test name | `FULL_SUITE_REQUIRED` | catalogue ambiguous |
| empty command array | `FULL_SUITE_SELECTED` if remaining catalogue trustworthy; otherwise required | mapping incomplete |
| wrapper/unmapped/ambiguous test command | `FULL_SUITE_SELECTED` | omitted wrapper test cannot be proven unaffected |
| File API reply absent | `FULL_SUITE_SELECTED` | target/source mapping unavailable |
| ambiguous reply index | `FULL_SUITE_SELECTED` | configuration snapshot unclear |
| codemodel unsupported/malformed | `FULL_SUITE_SELECTED` | target graph untrusted |
| build/project root mismatch | `FULL_SUITE_SELECTED` | paths may refer to different tree |
| multiple configs without explicit selection | `FULL_SUITE_SELECTED` | artifact identity ambiguous |
| target object missing | `FULL_SUITE_SELECTED` | graph incomplete |
| relevant `.d` file absent/malformed | `FULL_SUITE_SELECTED` | include evidence incomplete |
| stale `.d` detected | `FULL_SUITE_SELECTED` | dependencies may have changed |
| `.d` target cannot map uniquely | `FULL_SUITE_SELECTED` | translation-unit ownership unclear |
| changed source/header known | continue | evidence node exists |
| changed path unknown | `FULL_SUITE_SELECTED` | it may affect generation/configuration |
| changed `CMakeLists.txt`/`.cmake` | `FULL_SUITE_SELECTED` | build graph may change |
| changed test metadata/config | `FULL_SUITE_SELECTED` | registrations may change |
| affected generated source/custom command | `FULL_SUITE_SELECTED` | generation chain unsupported |
| graph cycle | traverse with visited set; fallback only if cycle type unsupported | cycles alone are not corruption |
| path escapes declared roots | usage/input error; no subset | unsafe normalization |
| input limit exceeded | full known suite or required | cannot safely finish analysis |
| unexpected exception | `INTERNAL_ERROR` | state unknown |

## 5. Unknown-path policy

MVP uses an allowlist mindset. A changed path is known if it appears as one or more of:

- a codemodel source;
- a dependency prerequisite inside project root;
- an explicitly recognized build configuration file that causes fallback;
- a registered input category intentionally modeled.

Do not infer safety from extensions such as `.md`, `.txt`, or image files. A documentation file can still be consumed by a generator or test. Optional ignore rules are postponed until they can be configured and audited.

## 6. Deleted and renamed paths

Deleted files may not appear in current metadata. Therefore any changed path that does not map in current evidence triggers full selection. For renames, callers should supply both old and new paths; if the old path is unknown, fallback remains correct.

## 7. Completeness model

An evidence source is not “complete” merely because some files were found.

### CTest completeness

Requires a valid catalogue and a unique supported mapping for every registered test.

### Codemodel completeness

Requires every referenced in-scope target object to be present and supported under exactly one configuration.

### Dependency completeness

Requires supported `.d` evidence for every compiled translation unit that could participate in the relevant target/test graph, unless a narrower completeness proof is implemented and tested.

Conservative MVP recommendation: require global dependency completeness for all in-scope project targets before allowing any subset. This may over-fallback but is easier to defend than partial completeness.

## 8. Freshness model

The tool cannot cryptographically prove correspondence between metadata and source state. It should:

- compare build/project roots with codemodel paths;
- optionally flag dependency files older than their known prerequisites;
- expose a verbose summary of evidence files;
- document that users must generate metadata in the same build workflow immediately before analysis.

If detectable staleness exists, fall back. If no staleness is detected, avoid claiming absolute freshness.

## 9. Explanation requirements

Every subset-selected test needs one chain:

```text
changed path
  → affected translation unit
  → owning/affected target
  → executable artifact mapping
  → registered test
```

Each edge must come from a named evidence source. Human output may abbreviate file paths, but verbose mode should identify source metadata.

Full-suite fallback explanations must include:

- category of uncertainty;
- affected input/path/target when available;
- why it prevents safe omission;
- caller action.

## 10. Exit and output rule

Safety status is decided before stdout is finalized. Names mode emits:

- subset names for `SUBSET_SELECTED`;
- every catalogue name for `FULL_SUITE_SELECTED`;
- no names for `FULL_SUITE_REQUIRED`.

stderr always announces fallback/required states even without verbose mode.

## 11. Forbidden shortcuts

- treating a missing `.d` file as “no dependencies”;
- basename-only executable matching;
- selecting tests by `*_test.cpp` filename conventions;
- silently ignoring unknown JSON types for required members;
- continuing after unsupported major versions;
- assuming unknown changed extensions are harmless;
- outputting a partial result alongside an internal error;
- labeling fallback as precise selection;
- launching a tool to fill missing evidence.

## 12. Safety review gate

Before submission, construct a table of every place code can set an “incomplete” flag and verify that each flag reaches the final outcome decision. Add mutation tests manually by removing one evidence element at a time. Any missing edge that still produces `SUBSET_SELECTED` is a release blocker.
