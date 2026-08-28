#define D2T_TESTING
#include "../diff2test.cpp"
#include <fstream>
#include <iostream>

namespace {
int failures = 0;

void check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}

void put(const std::filesystem::path& path, std::string_view text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << text;
}

d2t::cli::AnalyzeOptions opts(const std::filesystem::path& root) {
    d2t::cli::AnalyzeOptions options;
    options.project_root = root / "project";
    options.build_root = root / "build";
    options.changed_files = (root / "changed.txt").string();
    options.cmake_reply = root / "build/.cmake/api/v1/reply";
    options.ctest_info = root / "build/ctest-info.json";
    options.dep_list = root / "build/deps.txt";
    options.format = "names";
    return options;
}

void fixture(const std::filesystem::path& root) {
    std::filesystem::remove_all(root);
    const auto project = root / "project";
    const auto build = root / "build";
    const auto reply = build / ".cmake/api/v1/reply";

    std::filesystem::create_directories(project / "src");
    std::filesystem::create_directories(project / "tests");
    std::filesystem::create_directories(project / "include");
    std::filesystem::create_directories(reply);

    put(reply / "index-a.json",
        R"({"reply":{"codemodel-v2":{"jsonFile":"codemodel.json","kind":"codemodel","version":{"major":2}}}})");
    put(reply / "codemodel.json",
        std::string("{\"paths\":{\"source\":\"") + project.string() +
            "\",\"build\":\"" + build.string() +
            "\"},\"configurations\":[{\"name\":\"\",\"targets\":["
            "{\"id\":\"alpha::x\",\"jsonFile\":\"target-alpha.json\"},"
            "{\"id\":\"alpha_test::x\",\"jsonFile\":\"target-alpha-test.json\"}]}]}" );
    put(reply / "target-alpha.json",
        R"({"id":"alpha::x","name":"alpha","type":"STATIC_LIBRARY","sources":[{"path":"src/alpha.cpp"}],"artifacts":[],"dependencies":[]})");
    put(reply / "target-alpha-test.json",
        R"({"id":"alpha_test::x","name":"alpha_test","type":"EXECUTABLE","sources":[{"path":"tests/alpha_test.cpp"}],"artifacts":[{"path":"alpha_test"}],"dependencies":[{"id":"alpha::x"}]})");
    put(build / "ctest-info.json",
        std::string("{\"kind\":\"ctestInfo\",\"version\":{\"major\":1},\"tests\":[{\"name\":\"AlphaTest\",\"command\":[\"") +
            (build / "alpha_test").string() + "\"]}]}" );

    put(build / "CMakeFiles/alpha.dir/src/alpha.cpp.o.d",
        std::string("CMakeFiles/alpha.dir/src/alpha.cpp.o: ") +
            (project / "src/alpha.cpp").string() + " " +
            (project / "include/alpha.hpp").string() + "\n");
    put(build / "CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d",
        std::string("CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o: ") +
            (project / "tests/alpha_test.cpp").string() + " " +
            (project / "include/alpha.hpp").string() + "\n");
    put(build / "deps.txt",
        "CMakeFiles/alpha.dir/src/alpha.cpp.o.d\n"
        "CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d\n");
    put(root / "changed.txt", "include/alpha.hpp\n");
}

void expect_full(const d2t::analysis::Result& result, const char* name) {
    check(result.outcome == d2t::core::Outcome::FullSuiteSelected, name);
    check(result.selected_tests == std::vector<std::string>{"AlphaTest"}, name);
}

void expect_required(const d2t::analysis::Result& result, const char* name) {
    check(result.outcome == d2t::core::Outcome::FullSuiteRequired, name);
    check(result.selected_tests.empty(), name);
}
}  // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "d2t-impact-tests";
    auto options = opts(root);

    fixture(root);
    const auto good = d2t::analysis::analyze(options);
    check(good.outcome == d2t::core::Outcome::SubsetSelected, "subset status");
    check(good.selected_tests == std::vector<std::string>{"AlphaTest"}, "subset contents");

    fixture(root);
    put(root / "build/deps.txt", "CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d\n");
    expect_full(d2t::analysis::analyze(options), "missing dependency coverage falls back");

    fixture(root);
    put(root / "build/CMakeFiles/alpha.dir/src/alpha.cpp.o.d", "this is not a dependency rule\n");
    expect_full(d2t::analysis::analyze(options), "malformed dependency file falls back");

    fixture(root);
    std::filesystem::remove(root / "build/deps.txt");
    expect_full(d2t::analysis::analyze(options), "missing dependency list falls back");

    fixture(root);
    put(root / "build/deps.txt",
        "CMakeFiles/alpha.dir/src/alpha.cpp.o.d\n"
        "CMakeFiles/alpha.dir/src/alpha.cpp.o.d\n"
        "CMakeFiles/alpha_test.dir/tests/alpha_test.cpp.o.d\n");
    expect_full(d2t::analysis::analyze(options), "duplicate dependency evidence falls back");

    fixture(root);
    put(root / "changed.txt", "docs/unknown.md\n");
    expect_full(d2t::analysis::analyze(options), "unknown changed path falls back");

    fixture(root);
    put(root / "changed.txt", "CMakeLists.txt\n");
    expect_full(d2t::analysis::analyze(options), "build configuration change falls back");

    fixture(root);
    put(root / "build/ctest-info.json", "{");
    expect_required(d2t::analysis::analyze(options), "malformed CTest catalogue requires full suite");

    fixture(root);
    put(root / "build/ctest-info.json",
        R"({"kind":"ctestInfo","version":{"major":2},"tests":[{"name":"AlphaTest","command":["/tmp/alpha_test"]}]})");
    expect_required(d2t::analysis::analyze(options), "unsupported CTest major requires full suite");

    fixture(root);
    std::filesystem::remove(root / "build/ctest-info.json");
    expect_required(d2t::analysis::analyze(options), "missing CTest catalogue requires full suite");

    fixture(root);
    const auto build = root / "build";
    put(build / "ctest-info.json",
        std::string("{\"kind\":\"ctestInfo\",\"version\":{\"major\":1},\"tests\":[{\"name\":\"AlphaTest\",\"command\":[\"/bin/sh\",\"") +
            (build / "alpha_test").string() + "\"]}]}" );
    expect_full(d2t::analysis::analyze(options), "wrapper-style test command falls back");

    fixture(root);
    put(root / "build/.cmake/api/v1/reply/index-a.json", "{");
    expect_full(d2t::analysis::analyze(options), "malformed CMake index falls back");

    fixture(root);
    std::filesystem::remove(root / "build/.cmake/api/v1/reply/target-alpha.json");
    expect_full(d2t::analysis::analyze(options), "missing target object falls back");

    fixture(root);
    put(root / "build/.cmake/api/v1/reply/index-b.json",
        R"({"reply":{"codemodel-v2":{"jsonFile":"codemodel.json","kind":"codemodel","version":{"major":2}}}})");
    expect_full(d2t::analysis::analyze(options), "ambiguous CMake index falls back");

    fixture(root);
    put(root / "build/.cmake/api/v1/reply/codemodel.json",
        std::string("{\"paths\":{\"source\":\"") + (root / "other-project").string() +
            "\",\"build\":\"" + (root / "build").string() +
            "\"},\"configurations\":[{\"name\":\"\",\"targets\":["
            "{\"id\":\"alpha::x\",\"jsonFile\":\"target-alpha.json\"},"
            "{\"id\":\"alpha_test::x\",\"jsonFile\":\"target-alpha-test.json\"}]}]}" );
    expect_full(d2t::analysis::analyze(options), "project-root mismatch falls back");

    fixture(root);
    put(root / "build/.cmake/api/v1/reply/target-alpha-test.json",
        R"({"id":"alpha_test::x","name":"alpha_test","type":"EXECUTABLE","sources":[{"path":"tests/alpha_test.cpp"}],"artifacts":[{"path":"alpha_test"}],"dependencies":[{"id":"missing::x"}]})");
    expect_full(d2t::analysis::analyze(options), "unknown target dependency falls back");

    std::filesystem::remove_all(root);
    if (failures == 0) {
        std::cout << "impact_tests: all checks passed\n";
        return 0;
    }
    std::cerr << "impact_tests: " << failures << " failure(s)\n";
    return 1;
}
