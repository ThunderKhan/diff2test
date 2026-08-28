#define D2T_TESTING
#include "../diff2test.cpp"

#include <iostream>

namespace {
int failures = 0;

void check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}
}  // namespace

int main() {
    const std::filesystem::path project = "/work/project";
    const std::filesystem::path build = "/work/project-build";

    {
        const auto result = d2t::path::normalize("src/main.cpp", project, build);
        check(result.ok() && result.value->root_class == d2t::path::RootClass::Project, "relative project path");
        check(result.value->display == std::filesystem::path("src/main.cpp"), "project display path");
    }
    {
        const auto result = d2t::path::normalize("./src/../include/a.hpp", project, build);
        check(result.ok() && result.value->display == std::filesystem::path("include/a.hpp"), "dot normalization");
    }
    {
        const auto result = d2t::path::normalize("/work/project/src/main.cpp", project, build);
        check(result.ok() && result.value->root_class == d2t::path::RootClass::Project, "absolute project path");
    }
    {
        const auto result = d2t::path::normalize("/work/project-build/CMakeFiles/a.o", project, build);
        check(result.ok() && result.value->root_class == d2t::path::RootClass::Build, "absolute build path");
    }
    {
        const auto result = d2t::path::normalize("../outside.txt", project, build);
        check(!result.ok(), "relative escape rejected");
    }
    {
        const auto result = d2t::path::normalize("/usr/include/vector", project, build);
        check(!result.ok(), "external rejected by default");
    }
    {
        const auto result = d2t::path::normalize("/usr/include/vector", project, build, true);
        check(result.ok() && result.value->root_class == d2t::path::RootClass::External, "external allowed for dependency evidence");
    }
    {
        const auto result = d2t::path::normalize("", project, build);
        check(!result.ok(), "empty rejected");
    }
    {
        const auto result = d2t::path::normalize("src/main.cpp", "relative/project", build);
        check(!result.ok(), "relative root rejected");
    }
    {
        const auto result = d2t::path::normalize("/work/projectish/file", project, build);
        check(!result.ok(), "component prefix not string prefix");
    }

    if (failures == 0) {
        std::cout << "path_tests: all checks passed\n";
        return 0;
    }
    std::cerr << "path_tests: " << failures << " failure(s)\n";
    return 1;
}
