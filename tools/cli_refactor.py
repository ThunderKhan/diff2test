from pathlib import Path

# Runtime implementation: replace only the CLI layer; keep the analysis engine intact
# except for clearer missing-default diagnostics.
p = Path('diff2test.cpp')
s = p.read_text()
start = s.index('namespace d2t::cli {')
end = s.index('\nnamespace d2t::analysis {', start)
new_cli = r'''namespace d2t::cli {
struct AnalyzeOptions {
  std::filesystem::path project_root, build_root, cmake_reply, ctest_info,
      dep_list;
  std::string changed_files;
  std::optional<std::filesystem::path> cmake_index;
  std::optional<std::string> configuration;
  std::string format = "human";
  bool explain = false, verbose = false;
};
struct ParseResult {
  bool ok = false;
  AnalyzeOptions options;
  std::string error;
};
void print_help(std::ostream &out) {
  out << "diff2test - conservative C++ test-impact analysis\n\nUsage:\n  "
         "diff2test --help\n  diff2test --version\n  diff2test analyze "
         "[project-root] [options]\n\nDefaults:\n  project-root: .\n  "
         "build root: <project-root>/build\n  changed paths: stdin\n  "
         "CMake reply: <build-root>/.cmake/api/v1/reply\n  CTest catalogue: "
         "<build-root>/ctest-info.json\n  dependency list: <build-root>/deps.txt\n\n"
         "Overrides:\n  --project-root <dir>\n  --build <dir>\n  --build-root "
         "<dir>\n  --changed-files <file|->\n  --cmake-reply <dir>\n  "
         "--ctest-info <file>\n  --dep-list <file>\n  --cmake-index <file>\n  "
         "--configuration <name>\n  --format <human|names>\n  --explain\n  "
         "--verbose\n\nCMake, CTest, Git, and the compiler may generate input "
         "externally, but diff2test never launches them or any other program at "
         "runtime.\n";
}
ParseResult parse_analyze(int argc, char **argv) {
  ParseResult r;
  std::map<std::string, std::string> v;
  std::set<std::string> f;
  std::optional<std::string> positional_project_root;
  const std::set<std::string> vo = {
      "--project-root", "--build",         "--build-root",
      "--changed-files", "--cmake-reply", "--cmake-index",
      "--ctest-info",    "--dep-list",    "--configuration",
      "--format"};
  const std::set<std::string> fo = {"--explain", "--verbose"};
  for (int i = 2; i < argc; ++i) {
    std::string a = argv[i];
    if (vo.contains(a)) {
      if (v.contains(a)) {
        r.error = "repeated option: " + a;
        return r;
      }
      if (i + 1 >= argc) {
        r.error = "missing value for option: " + a;
        return r;
      }
      v.emplace(a, argv[++i]);
    } else if (fo.contains(a)) {
      if (!f.insert(a).second) {
        r.error = "repeated option: " + a;
        return r;
      }
    } else if (!a.empty() && a.front() == '-') {
      r.error = "unknown option: " + a;
      return r;
    } else {
      if (positional_project_root) {
        r.error = "multiple positional project roots";
        return r;
      }
      positional_project_root = a;
    }
  }
  if (positional_project_root && v.contains("--project-root")) {
    r.error = "project root specified both positionally and with --project-root";
    return r;
  }
  if (v.contains("--build") && v.contains("--build-root")) {
    r.error = "build root specified with both --build and --build-root";
    return r;
  }
  if (v.contains("--format") && v["--format"] != "human" &&
      v["--format"] != "names") {
    r.error = "--format must be human or names";
    return r;
  }

  r.options.project_root = positional_project_root
                               ? std::filesystem::path(*positional_project_root)
                               : std::filesystem::path(v.contains("--project-root")
                                                           ? v["--project-root"]
                                                           : ".");
  if (v.contains("--build"))
    r.options.build_root = v["--build"];
  else if (v.contains("--build-root"))
    r.options.build_root = v["--build-root"];
  else
    r.options.build_root = r.options.project_root / "build";

  r.options.changed_files =
      v.contains("--changed-files") ? v["--changed-files"] : "-";
  r.options.cmake_reply = v.contains("--cmake-reply")
                              ? std::filesystem::path(v["--cmake-reply"])
                              : r.options.build_root / ".cmake/api/v1/reply";
  r.options.ctest_info = v.contains("--ctest-info")
                             ? std::filesystem::path(v["--ctest-info"])
                             : r.options.build_root / "ctest-info.json";
  r.options.dep_list = v.contains("--dep-list")
                           ? std::filesystem::path(v["--dep-list"])
                           : r.options.build_root / "deps.txt";
  if (v.contains("--cmake-index"))
    r.options.cmake_index = v["--cmake-index"];
  if (v.contains("--configuration"))
    r.options.configuration = v["--configuration"];
  if (v.contains("--format"))
    r.options.format = v["--format"];
  r.options.explain = f.contains("--explain");
  r.options.verbose = f.contains("--verbose");
  r.ok = true;
  return r;
}
} // namespace d2t::cli'''
s = s[:start] + new_cli + s[end:]

old = 'Result analyze(const cli::AnalyzeOptions &opt) {\n  Result out;\n  auto cat = ctest::load(opt.ctest_info);'
new = '''Result analyze(const cli::AnalyzeOptions &opt) {
  Result out;
  std::error_code metadata_ec;
  if (!std::filesystem::exists(opt.ctest_info, metadata_ec) || metadata_ec) {
    out.outcome = core::Outcome::FullSuiteRequired;
    out.reasons.push_back("CTest catalogue not found at " + opt.ctest_info.string());
    return out;
  }
  auto cat = ctest::load(opt.ctest_info);'''
if old in s:
    s = s.replace(old, new, 1)
elif 'CTest catalogue not found at ' not in s:
    raise SystemExit('analysis entry not in expected state')

old = '  auto model = cmake::load(opt.cmake_reply, opt.cmake_index, opt.configuration);\n  if (!model.ok())\n    return full(model.error);'
new = '''  if (!std::filesystem::is_directory(opt.cmake_reply))
    return full("CMake reply directory not found at " + opt.cmake_reply.string());
  auto model = cmake::load(opt.cmake_reply, opt.cmake_index, opt.configuration);
  if (!model.ok())
    return full(model.error);'''
if old in s:
    s = s.replace(old, new, 1)
elif 'CMake reply directory not found at ' not in s:
    raise SystemExit('cmake load not in expected state')

old = '  std::string err;\n  auto deps = load_dependencies(opt.dep_list, *model.model, declared_project,\n                                declared_build, err);'
new = '''  std::string err;
  if (!std::filesystem::exists(opt.dep_list))
    return full("dependency list not found at " + opt.dep_list.string());
  auto deps = load_dependencies(opt.dep_list, *model.model, declared_project,
                                declared_build, err);'''
if old in s:
    s = s.replace(old, new, 1)
elif 'dependency list not found at ' not in s:
    raise SystemExit('dependency load not in expected state')
p.write_text(s)

# README: put the convention-based workflow first while retaining the fully explicit example below.
p = Path('README.md')
s = p.read_text()
marker = '## Quick start\n'
insert = '''## Quick start

> **diff2test never runs Git, CMake, or CTest; it consumes changed paths and metadata those tools already produced.**

For the conventional `build/` layout, generate the catalogue and dependency-file list externally, then compose changed paths through stdin:

```bash
ctest --test-dir build --show-only=json-v1 > build/ctest-info.json
find build -type f -name '*.o.d' -printf '%P\\n' | sort > build/deps.txt
git diff --name-only HEAD~1 | ./build/diff2test analyze .
```

The shell launches `git`, `ctest`, and `find`; `diff2test` launches nothing. The shorthand defaults to `./build`, stdin, `build/.cmake/api/v1/reply`, `build/ctest-info.json`, and `build/deps.txt`. Every default has an explicit override.

'''
if 'The shorthand defaults to `./build`' not in s:
    s = s.replace(marker, insert, 1)
old_cli = '''diff2test analyze \\
  --project-root <dir> \\
  --build-root <dir> \\
  --changed-files <file|-> \\
  --cmake-reply <dir> \\
  --ctest-info <file> \\
  --dep-list <file> \\
  [--cmake-index <file>] \\
  [--configuration <name>] \\
  [--format human|names] \\
  [--explain] \\
  [--verbose]'''
new_cli_doc = '''diff2test analyze [project-root] \\
  [--build <dir> | --build-root <dir>] \\
  [--changed-files <file|->] \\
  [--cmake-reply <dir>] \\
  [--ctest-info <file>] \\
  [--dep-list <file>] \\
  [--cmake-index <file>] \\
  [--configuration <name>] \\
  [--format human|names] \\
  [--explain] \\
  [--verbose]'''
if old_cli in s:
    s = s.replace(old_cli, new_cli_doc, 1)
p.write_text(s)

# CLI contract: update the frozen interface without changing output/exit semantics.
p = Path('CLI-CONTRACT.md')
s = p.read_text()
s = s.replace('diff2test analyze [options]', 'diff2test analyze [project-root] [options]')
old = '''diff2test analyze \\
  --project-root <dir> \\
  --build-root <dir> \\
  --changed-files <file|-> \\
  --cmake-reply <dir> \\
  --ctest-info <file> \\
  --dep-list <file> \\
  [--cmake-index <file>] \\
  [--configuration <name>] \\
  [--format human|names] \\
  [--explain] \\
  [--verbose]'''
new = '''diff2test analyze [project-root] \\
  [--build <dir> | --build-root <dir>] \\
  [--changed-files <file|->] \\
  [--cmake-reply <dir>] \\
  [--ctest-info <file>] \\
  [--dep-list <file>] \\
  [--cmake-index <file>] \\
  [--configuration <name>] \\
  [--format human|names] \\
  [--explain] \\
  [--verbose]'''
s = s.replace(old, new)
s = s.replace('The kickoff metadata spike found non-compilation `.d` files such as `link.d` in the build tree. To avoid generator-specific guessing, the MVP deliberately requires an explicit `--dep-list`. Recursive `--dep-root` discovery is unsupported.', 'The kickoff metadata spike found non-compilation `.d` files such as `link.d` in the build tree. To avoid generator-specific guessing, the MVP deliberately does not recursively discover `.d` files. The conventional dependency-list path is `<build-root>/deps.txt`; `--dep-list` overrides it. Recursive `--dep-root` discovery is unsupported.')
s = s.replace('| `--project-root <dir>` | yes | top-level source root |', '| positional `[project-root]` | no | top-level source root; defaults to `.` |\n| `--project-root <dir>` | no | explicit override for the project root; cannot be combined with the positional root |')
s = s.replace('| `--build-root <dir>` | yes | build-tree root |', '| `--build <dir>` | no | convenience alias for the build root |\n| `--build-root <dir>` | no | build-tree root; defaults to `<project-root>/build`; cannot be combined with `--build` |')
s = s.replace('| `--changed-files <file|->` | yes | newline-delimited changed paths; `-` reads stdin |', '| `--changed-files <file|->` | no | newline-delimited changed paths; omitted or `-` reads stdin |')
s = s.replace('| `--cmake-reply <dir>` | yes | File API v1 reply directory |', '| `--cmake-reply <dir>` | no | File API v1 reply directory; defaults to `<build-root>/.cmake/api/v1/reply` |')
s = s.replace('| `--ctest-info <file>` | yes | pre-generated CTest JSON |', '| `--ctest-info <file>` | no | pre-generated CTest JSON; defaults to `<build-root>/ctest-info.json` |')
s = s.replace('| `--dep-list <file>` | yes | newline-delimited explicit list of compiler dependency files |', '| `--dep-list <file>` | no | newline-delimited explicit list of compiler dependency files; defaults to `<build-root>/deps.txt` |')
s = s.replace('diff2test: FULL_SUITE_REQUIRED: cannot open file: <ctest-json-path>', 'diff2test: FULL_SUITE_REQUIRED: CTest catalogue not found at <ctest-json-path>')
s = s.replace('- required analyze options;\n- optional analyze flags;', '- convention-based defaults and explicit overrides;')
s = s.replace('- missing required option/value;\n', '- missing option value;\n- conflicting positional/`--project-root` declarations;\n- conflicting `--build`/`--build-root` declarations;\n')
s = s.replace('The MVP does not attempt TTY detection for `--changed-files -`.', 'The MVP does not attempt TTY detection. Omitted `--changed-files` and explicit `--changed-files -` both consume stdin.')
s = s.replace('The command names, required inputs, output modes, and safety exit statuses are frozen for the hackathon MVP.', 'The command names, convention-based defaults, explicit overrides, output modes, and safety exit statuses are frozen for the hackathon MVP.')
p.write_text(s)

# Input spec: defaults are deterministic conventions, never searches.
p = Path('INPUT-SPEC.md')
s = p.read_text()
s = s.replace('Exactly one of:\n\n- `--changed-files <path>`\n- `--changed-files -` for stdin', 'Default source: stdin.\n\nOverrides:\n\n- `--changed-files <path>`\n- `--changed-files -` explicitly selects stdin')
s = s.replace('`--cmake-reply <directory>` points to a CMake File API v1 reply directory, normally:', 'The default CMake File API v1 reply directory is:')
s = s.replace('The supported input is:\n\n```text\n--dep-list <file>\n```\n\nThe list contains one dependency-file path per line.', 'The supported input is a dependency-list file. Its conventional default is `<build-root>/deps.txt`; `--dep-list <file>` overrides that path. diff2test does not recursively scan the build tree for `.d` files.\n\nThe list contains one dependency-file path per line.')
s = s.replace('`--ctest-info <path>` identifies a file previously produced outside diff2test by:', 'The default catalogue path is `<build-root>/ctest-info.json`; `--ctest-info <path>` overrides it. The file is produced outside diff2test by:')
s = s.replace('- `--project-root` and `--build-root` must exist and be directories;', '- the positional project root defaults to `.`; `--project-root` is an explicit alternative;\n- the build root defaults to `<project-root>/build`; `--build` or `--build-root` overrides it;\n- project and build roots must exist and be directories;')
s = s.replace('| changed paths missing/unreadable/empty | usage error / no subset |', '| changed paths unreadable/empty (including empty stdin) | usage error / no subset |')
if '## 11. Future inputs not in MVP' in s and 'Deterministic conventions' not in s:
    s = s.replace('## 11. Future inputs not in MVP', '## 11. Deterministic conventions\n\nThe shorthand interface composes paths only from the declared/default project and build roots. It does not probe alternative build directories, search for CTest catalogues, or recursively discover `.d` files. Missing conventional metadata follows the same conservative fallback rules as missing explicitly named metadata.\n\n## 12. Future inputs not in MVP')
p.write_text(s)

# Small documentation addenda.
additions = {
    'STDLIB.md': '''\n## Convention-based CLI composition\n\nThe shorthand interface uses `std::filesystem` for deterministic path composition and `std::cin` for changed paths supplied through a Unix pipeline. No subprocess facility is introduced: `system`, `popen`, `fork`, `exec`, `posix_spawn`, and equivalents remain absent from the runtime. Git, CMake, CTest, the compiler, and shell utilities are caller-side tools only.\n''',
    'SUBMISSION.md': '''\n## Preferred shorthand workflow\n\nThe primary demonstration uses `git diff --name-only HEAD~1 | diff2test analyze .` after required metadata has been generated externally. This does not make Git a runtime dependency: the caller's shell launches Git and writes newline-delimited paths to stdin; `diff2test` only consumes stdin and existing metadata files. Conventional metadata locations are deterministic defaults, with all explicit legacy flags retained as overrides.\n''',
    'DEMO-SCRIPT.md': '''\n## Shorthand CLI beat\n\nAfter showing that the CTest catalogue and dependency list were generated externally, run `git diff --name-only HEAD~1 | ./build/diff2test analyze .`. Say: “diff2test never runs Git, CMake, or CTest; it consumes changed paths and metadata those tools already produced.”\n'''
}
for name, extra in additions.items():
    q = Path(name)
    text = q.read_text()
    heading = extra.strip().splitlines()[0]
    if heading not in text:
        q.write_text(text.rstrip() + '\n' + extra)

# Permanent CI coverage for shorthand/default behavior and explicit compatibility.
p = Path('.github/workflows/ci.yml')
s = p.read_text()
marker = '      - name: Verify real shared-header multi-test selection\n'
block = r'''      - name: Verify shorthand defaults and legacy equivalence
        shell: bash
        run: |
          explicit=$(build/diff2test analyze \
            --project-root "$(pwd)/fixture" \
            --build-root "$(pwd)/fixture/build" \
            --changed-files fixture/changed.txt \
            --cmake-reply fixture/build/.cmake/api/v1/reply \
            --ctest-info fixture/build/ctest-info.json \
            --dep-list fixture/build/deps.txt \
            --format names)

          shorthand=$(printf 'include/alpha.hpp\n' | build/diff2test analyze "$(pwd)/fixture" --format names)
          alias_form=$(printf 'include/alpha.hpp\n' | build/diff2test analyze "$(pwd)/fixture" --build "$(pwd)/fixture/build" --format names)
          file_override=$(build/diff2test analyze "$(pwd)/fixture" --changed-files fixture/changed.txt --format names)

          test "$explicit" = 'AlphaTest'
          test "$shorthand" = "$explicit"
          test "$alias_form" = "$explicit"
          test "$file_override" = "$explicit"

          set +e
          printf 'include/alpha.hpp\n' | build/diff2test analyze "$(pwd)/fixture" \
            --ctest-info fixture/build/missing-default-catalogue.json \
            --format names >/tmp/missing-default.out 2>/tmp/missing-default.err
          missing_status=$?
          build/diff2test analyze . --project-root . >/dev/null 2>/tmp/root-conflict.err
          root_conflict=$?
          build/diff2test analyze . --build build --build-root build >/dev/null 2>/tmp/build-conflict.err
          build_conflict=$?
          set -e

          test "$missing_status" -eq 11
          test ! -s /tmp/missing-default.out
          grep -q 'FULL_SUITE_REQUIRED: CTest catalogue not found at' /tmp/missing-default.err
          test "$root_conflict" -eq 64
          test "$build_conflict" -eq 64

'''
if 'Verify shorthand defaults and legacy equivalence' not in s:
    if marker not in s:
        raise SystemExit('CI insertion marker not found')
    s = s.replace(marker, block + marker, 1)
p.write_text(s)
