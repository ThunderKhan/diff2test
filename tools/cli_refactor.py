from pathlib import Path

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

needle = 'Result analyze(const cli::AnalyzeOptions &opt) {\n  Result out;\n  auto cat = ctest::load(opt.ctest_info);'
repl = '''Result analyze(const cli::AnalyzeOptions &opt) {
  Result out;
  std::error_code metadata_ec;
  if (!std::filesystem::exists(opt.ctest_info, metadata_ec) || metadata_ec) {
    out.outcome = core::Outcome::FullSuiteRequired;
    out.reasons.push_back("CTest catalogue not found at " + opt.ctest_info.string());
    return out;
  }
  auto cat = ctest::load(opt.ctest_info);'''
if needle not in s:
    raise SystemExit('analysis entry needle not found')
s = s.replace(needle, repl, 1)

needle = '  auto model = cmake::load(opt.cmake_reply, opt.cmake_index, opt.configuration);\n  if (!model.ok())\n    return full(model.error);'
repl = '''  if (!std::filesystem::is_directory(opt.cmake_reply))
    return full("CMake reply directory not found at " + opt.cmake_reply.string());
  auto model = cmake::load(opt.cmake_reply, opt.cmake_index, opt.configuration);
  if (!model.ok())
    return full(model.error);'''
if needle not in s:
    raise SystemExit('cmake load needle not found')
s = s.replace(needle, repl, 1)

needle = '  std::string err;\n  auto deps = load_dependencies(opt.dep_list, *model.model, declared_project,\n                                declared_build, err);'
repl = '''  std::string err;
  if (!std::filesystem::exists(opt.dep_list))
    return full("dependency list not found at " + opt.dep_list.string());
  auto deps = load_dependencies(opt.dep_list, *model.model, declared_project,
                                declared_build, err);'''
if needle not in s:
    raise SystemExit('dep load needle not found')
s = s.replace(needle, repl, 1)
p.write_text(s)
