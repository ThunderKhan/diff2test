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
    std::ofstream out(path);
    out << text;
}
}  // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "d2t-cmake-tests";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    put(root / "index-a.json", R"({"reply":{"codemodel-v2":{"jsonFile":"codemodel.json","kind":"codemodel","version":{"major":2,"minor":8}}}})");
    put(root / "codemodel.json", R"({"paths":{"source":"/p","build":"/b"},"configurations":[{"name":"","targets":[{"id":"alpha::x","jsonFile":"target-alpha.json"},{"id":"alpha_test::x","jsonFile":"target-alpha-test.json"}]}]})");
    put(root / "target-alpha.json", R"({"id":"alpha::x","name":"alpha","type":"STATIC_LIBRARY","sources":[{"path":"src/alpha.cpp"}],"artifacts":[],"dependencies":[]})");
    put(root / "target-alpha-test.json", R"({"id":"alpha_test::x","name":"alpha_test","type":"EXECUTABLE","sources":[{"path":"tests/alpha.cpp"}],"artifacts":[{"path":"alpha_test"}],"dependencies":[{"id":"alpha::x"}]})");

    const auto model = d2t::cmake::load(root, std::nullopt, std::nullopt);
    check(model.ok() && model.model->targets.size() == 2, "load codemodel and targets");

    if (model.ok()) {
        d2t::ctest::Catalogue catalogue{{{"AlphaTest", {"/b/alpha_test"}}}};
        const auto mapped = d2t::mapping::map_tests(catalogue, *model.model, "/b");
        check(mapped.complete() && mapped.mappings.size() == 1, "exact artifact mapping");

        d2t::ctest::Catalogue basename_only{{{"AlphaTest", {"/other/alpha_test"}}}};
        check(!d2t::mapping::map_tests(basename_only, *model.model, "/b").complete(), "reject basename-only match");
    }

    put(root / "index-b.json", R"({"reply":{"codemodel-v2":{"jsonFile":"codemodel.json","kind":"codemodel","version":{"major":2}}}})");
    check(!d2t::cmake::load(root, std::nullopt, std::nullopt).ok(), "ambiguous index falls back");
    std::filesystem::remove(root / "index-b.json");

    put(root / "index-a.json", R"({"reply":{"codemodel-v2":{"jsonFile":"../escape.json","kind":"codemodel","version":{"major":2}}}})");
    check(!d2t::cmake::load(root, std::nullopt, std::nullopt).ok(), "reject reply path traversal");

    std::filesystem::remove_all(root);
    if (failures == 0) {
        std::cout << "cmake_tests: all checks passed\n";
        return 0;
    }
    return 1;
}
