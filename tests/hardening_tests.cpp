#define D2T_TESTING
#include "../diff2test.cpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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

struct TargetSpec {
    std::string id;
    std::string name;
    std::string type;
    std::string source;
    std::string artifact;
    std::vector<std::string> dependencies;
};

std::string json_quote(const std::string& value) {
    return "\"" + value + "\"";
}

std::string target_json(const TargetSpec& target) {
    std::string json = std::string("{\"id\":") + json_quote(target.id) +
        ",\"name\":" + json_quote(target.name) +
        ",\"type\":" + json_quote(target.type) +
        ",\"sources\":[{\"path\":" + json_quote(target.source) + "}]";
    if (target.artifact.empty()) json += ",\"artifacts\":[]";
    else json += std::string(",\"artifacts\":[{\"path\":") + json_quote(target.artifact) + "}]";
    json += ",\"dependencies\":[";
    for (std::size_t i = 0; i < target.dependencies.size(); ++i) {
        if (i != 0) json += ',';
        json += std::string("{\"id\":") + json_quote(target.dependencies[i]) + "}";
    }
    json += "]}";
    return json;
}

void complex_fixture(const std::filesystem::path& root, bool reverse_metadata_order) {
    std::filesystem::remove_all(root);
    const auto project = root / "project";
    const auto build = root / "build";
    const auto reply = build / ".cmake/api/v1/reply";

    const std::vector<TargetSpec> targets = {
        {"core::x", "core", "STATIC_LIBRARY", "src/core.cpp", "", {}},
        {"left::x", "left", "STATIC_LIBRARY", "src/left.cpp", "", {"core::x"}},
        {"right::x", "right", "STATIC_LIBRARY", "src/right.cpp", "", {"core::x"}},
        {"join::x", "join", "STATIC_LIBRARY", "src/join.cpp", "", {"left::x", "right::x"}},
        {"cycle_a::x", "cycle_a", "STATIC_LIBRARY", "src/cycle_a.cpp", "", {"join::x", "cycle_b::x"}},
        {"cycle_b::x", "cycle_b", "STATIC_LIBRARY", "src/cycle_b.cpp", "", {"cycle_a::x"}},
        {"deep_test::x", "deep_test", "EXECUTABLE", "tests/deep_test.cpp", "deep_test", {"cycle_a::x"}},
        {"other::x", "other", "STATIC_LIBRARY", "src/other.cpp", "", {}},
        {"other_test::x", "other_test", "EXECUTABLE", "tests/other_test.cpp", "other_test", {"other::x"}}
    };

    put(project / "include/common.hpp", "#pragma once\n");
    for (const auto& target : targets) put(project / target.source, "int fixture_translation_unit;\n");

    put(reply / "index-a.json",
        R"({"reply":{"codemodel-v2":{"jsonFile":"codemodel.json","kind":"codemodel","version":{"major":2}}}})");

    std::vector<std::size_t> order;
    for (std::size_t i = 0; i < targets.size(); ++i) order.push_back(i);
    if (reverse_metadata_order) std::reverse(order.begin(), order.end());

    std::string codemodel = std::string("{\"paths\":{\"source\":") + json_quote(project.string()) +
        ",\"build\":" + json_quote(build.string()) +
        "},\"configurations\":[{\"name\":\"\",\"targets\":[";
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (i != 0) codemodel += ',';
        const auto& target = targets[order[i]];
        codemodel += std::string("{\"id\":") + json_quote(target.id) +
            ",\"jsonFile\":" + json_quote("target-" + target.name + ".json") + "}";
        put(reply / ("target-" + target.name + ".json"), target_json(target));
    }
    codemodel += "]}]}";
    put(reply / "codemodel.json", codemodel);

    const std::string deep_command = (build / "deep_test").string();
    const std::string other_command = (build / "other_test").string();
    std::string ctest_json = "{\"kind\":\"ctestInfo\",\"version\":{\"major\":1},\"tests\":[";
    if (reverse_metadata_order) {
        ctest_json += std::string("{\"name\":\"OtherTest\",\"command\":[") + json_quote(other_command) + "]},";
        ctest_json += std::string("{\"name\":\"DeepTest\",\"command\":[") + json_quote(deep_command) + "]}";
    } else {
        ctest_json += std::string("{\"name\":\"DeepTest\",\"command\":[") + json_quote(deep_command) + "]},";
        ctest_json += std::string("{\"name\":\"OtherTest\",\"command\":[") + json_quote(other_command) + "]}";
    }
    ctest_json += "]}";
    put(build / "ctest-info.json", ctest_json);

    std::vector<std::string> dep_entries;
    for (const auto& target : targets) {
        const auto source = project / target.source;
        const auto dep_path = build / "CMakeFiles" / (target.name + ".dir") / (target.source + ".o.d");
        std::string rule = "CMakeFiles/" + target.name + ".dir/" + target.source + ".o: " + source.string();
        if (target.name == "core") rule += " " + (project / "include/common.hpp").string();
        rule += "\n";
        put(dep_path, rule);
        dep_entries.push_back(dep_path.lexically_relative(build).generic_string());
    }
    if (reverse_metadata_order) std::reverse(dep_entries.begin(), dep_entries.end());
    std::string dep_list;
    for (const auto& entry : dep_entries) dep_list += entry + "\n";
    put(build / "deps.txt", dep_list);
    put(root / "changed.txt", reverse_metadata_order ? "include/common.hpp\ninclude/common.hpp\n" : "include/common.hpp\n");
}

d2t::cli::AnalyzeOptions opts(const std::filesystem::path& root) {
    d2t::cli::AnalyzeOptions options;
    options.project_root = root / "project";
    options.build_root = root / "build";
    options.changed_files = (root / "changed.txt").string();
    options.cmake_reply = root / "build/.cmake/api/v1/reply";
    options.ctest_info = root / "build/ctest-info.json";
    options.dep_list = root / "build/deps.txt";
    options.format = "human";
    options.explain = true;
    return options;
}

bool contains_step(const std::vector<std::string>& steps, std::string_view value) {
    return std::find(steps.begin(), steps.end(), value) != steps.end();
}
}  // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "d2t-hardening-tests";

    complex_fixture(root, false);
    const auto first = d2t::analysis::analyze(opts(root));
    check(first.outcome == d2t::core::Outcome::SubsetSelected, "complex graph selects subset");
    check(first.selected_tests == std::vector<std::string>{"DeepTest"}, "diamond/cycle selects only deep test");
    check(first.explanations.contains("DeepTest"), "complex graph explanation exists");
    if (first.explanations.contains("DeepTest")) {
        const auto& steps = first.explanations.at("DeepTest");
        check(contains_step(steps, "changed path: include/common.hpp"), "explanation contains changed path");
        check(contains_step(steps, "translation unit: src/core.cpp"), "explanation contains origin translation unit");
        check(contains_step(steps, "owning target: core"), "explanation contains owning target");
        check(contains_step(steps, "dependent target: join"), "diamond converges through join");
        check(contains_step(steps, "dependent target: cycle_a"), "cycle traversal reaches cycle target");
        check(contains_step(steps, "dependent target: deep_test"), "chain reaches test target");
        check(steps.back() == "registered test: DeepTest", "explanation ends at registered test");
    }

    const auto first_tests = first.selected_tests;
    const auto first_explanations = first.explanations;

    complex_fixture(root, true);
    const auto reordered = d2t::analysis::analyze(opts(root));
    check(reordered.outcome == d2t::core::Outcome::SubsetSelected, "reordered metadata still selects subset");
    check(reordered.selected_tests == first_tests, "selection invariant to metadata ordering");
    check(reordered.explanations == first_explanations, "explanations invariant to metadata ordering");

    const auto repeated = d2t::analysis::analyze(opts(root));
    check(repeated.selected_tests == reordered.selected_tests, "repeated selection deterministic");
    check(repeated.explanations == reordered.explanations, "repeated explanations deterministic");

    std::filesystem::remove_all(root);
    if (failures == 0) {
        std::cout << "hardening_tests: all checks passed\n";
        return 0;
    }
    std::cerr << "hardening_tests: " << failures << " failure(s)\n";
    return 1;
}
