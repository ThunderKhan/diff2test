#define D2T_TESTING
#include "../diff2test.cpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {
int failures = 0;

void check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}

void expect_error(std::string_view input, std::string_view code, const char* name) {
    const auto result = d2t::dep::parse(input);
    check(!result.ok() && result.error && result.error->code == code, name);
}
}  // namespace

int main() {
    {
        const auto result = d2t::dep::parse("obj.o: src.cpp include/a.hpp\n");
        check(result.ok() && result.rules.size() == 1, "simple rule");
        check(result.rules[0].targets == std::vector<std::string>{"obj.o"}, "simple target");
        check(result.rules[0].prerequisites.size() == 2, "simple prerequisites");
    }
    {
        const auto result = d2t::dep::parse("obj.o: src.cpp \\\n include/a.hpp\n");
        check(result.ok() && result.rules[0].prerequisites.size() == 2, "backslash newline continuation");
    }
    {
        const auto result = d2t::dep::parse("obj.o: path\\ with\\ spaces.hpp hash\\#name.hpp colon\\:name.hpp back\\\\slash.hpp\n");
        check(result.ok() && result.rules[0].prerequisites.size() == 4, "escaped tokens");
    }
    {
        const auto result = d2t::dep::parse("a.o b.o: src.cpp src.cpp\n");
        check(result.ok() && result.rules[0].targets.size() == 2, "multiple targets");
        check(result.rules[0].prerequisites.size() == 1, "deduplicate prerequisites");
    }
    {
        const auto result = d2t::dep::parse("header.hpp:\n");
        check(result.ok() && result.rules[0].prerequisites.empty(), "empty prerequisites");
    }
    {
        const auto result = d2t::dep::parse("obj.o: src.cpp\r\n");
        check(result.ok(), "crlf");
    }
    {
        const auto result = d2t::dep::parse("# comment\nobj.o: src.cpp # comment\n");
        check(result.ok() && result.rules.size() == 1, "comments");
    }
    {
        std::string large = "obj.o:";
        for (int i = 0; i < 10000; ++i) large += " include/header_" + std::to_string(i) + ".hpp";
        large += '\n';
        const auto result = d2t::dep::parse(large);
        check(result.ok(), "10k prerequisite rule parses");
        check(result.ok() && result.rules.size() == 1 && result.rules[0].prerequisites.size() == 10000,
              "10k prerequisite rule preserves all prerequisites");
    }

    expect_error("obj.o src.cpp\n", "DEP_MISSING_COLON", "missing colon");
    expect_error(": src.cpp\n", "DEP_EMPTY_TARGET", "empty target");
    expect_error("obj.o: src.cpp\\", "DEP_TRAILING_ESCAPE", "trailing escape");

    const std::string with_nul{"obj.o: a\0b", 10};
    expect_error(with_nul, "DEP_NUL", "nul rejected");

    if (failures == 0) {
        std::cout << "dep_tests: all checks passed\n";
        return 0;
    }
    std::cerr << "dep_tests: " << failures << " failure(s)\n";
    return 1;
}
