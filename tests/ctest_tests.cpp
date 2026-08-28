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
    const auto good = d2t::ctest::parse_catalogue(R"({"kind":"ctestInfo","version":{"major":1,"minor":0},"tests":[{"name":"Beta","command":["/b/beta"]},{"name":"Alpha","command":["/b/alpha","--x"]}]})");
    check(good.ok() && good.catalogue->tests.size() == 2, "valid catalogue");
    check(good.ok() && good.catalogue->tests[0].name == "Alpha", "deterministic sorting");

    check(!d2t::ctest::parse_catalogue(R"({"kind":"wrong","version":{"major":1},"tests":[]})").ok(), "wrong kind");
    check(!d2t::ctest::parse_catalogue(R"({"kind":"ctestInfo","version":{"major":2},"tests":[]})").ok(), "unsupported major");
    check(!d2t::ctest::parse_catalogue(R"({"kind":"ctestInfo","version":{"major":1},"tests":[{"name":"A","command":["x"]},{"name":"A","command":["y"]}]})").ok(), "duplicate name");
    check(!d2t::ctest::parse_catalogue(R"({"kind":"ctestInfo","version":{"major":1},"tests":[{"name":"","command":["x"]}]})").ok(), "empty name");
    check(!d2t::ctest::parse_catalogue(R"({"kind":"ctestInfo","version":{"major":1},"tests":[{"name":"A","command":[]}]})").ok(), "empty command");
    check(!d2t::ctest::parse_catalogue("{").ok(), "malformed JSON");

    if (failures == 0) {
        std::cout << "ctest_tests: all checks passed\n";
        return 0;
    }
    return 1;
}
