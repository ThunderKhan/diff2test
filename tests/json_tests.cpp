#define D2T_TESTING
#include "../diff2test.cpp"

#include <iostream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}

void expect_ok(std::string_view input, const char* name) {
    const auto result = d2t::json::parse(input);
    check(result.ok(), name);
    if (!result.ok() && result.error) {
        std::cerr << "  " << result.error->code << " at " << result.error->position.line << ':' << result.error->position.column << '\n';
    }
}

void expect_error(std::string_view input, std::string_view code, const char* name) {
    const auto result = d2t::json::parse(input);
    check(!result.ok(), name);
    check(result.error.has_value(), name);
    if (result.error) check(result.error->code == code, name);
}
}  // namespace

int main() {
    expect_ok("{}", "empty object");
    expect_ok("[]", "empty array");
    expect_ok("{\"n\":null,\"b\":true,\"f\":false}", "scalars");
    expect_ok("[0,-1,1.25,6.02e23,-2E-3]", "numbers");
    expect_ok("{\"nested\":[{\"x\":1}]}", "nesting");
    expect_ok("\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"", "escapes");
    expect_ok("\"\\u0041\"", "bmp unicode");
    expect_ok("\"\\uD83D\\uDE80\"", "surrogate pair");
    expect_ok(" \n\t {\"x\": 1 } \r\n", "whitespace");

    expect_error("\"unterminated", "JSON_UNTERMINATED_STRING", "unterminated string");
    expect_error("\"\\x\"", "JSON_INVALID_ESCAPE", "invalid escape");
    expect_error("\"\\uD800\"", "JSON_UNPAIRED_SURROGATE", "unpaired high surrogate");
    expect_error("\"\\uDC00\"", "JSON_UNPAIRED_SURROGATE", "unpaired low surrogate");
    expect_error("01", "JSON_INVALID_NUMBER", "leading zero");
    expect_error("[1,]", "JSON_TRAILING_COMMA", "array trailing comma");
    expect_error("{\"x\":1,}", "JSON_TRAILING_COMMA", "object trailing comma");
    expect_error("{\"x\" 1}", "JSON_EXPECTED_COLON", "missing colon");
    expect_error("[1 2]", "JSON_EXPECTED_COMMA", "missing comma");
    expect_error("true false", "JSON_TRAILING_DATA", "trailing data");
    expect_error("{\"x\":1,\"x\":2}", "JSON_DUPLICATE_KEY", "duplicate key");

    std::string invalid_utf8{"\xC0\xAF", 2};
    expect_error(invalid_utf8, "JSON_INVALID_UTF8", "invalid utf8");

    d2t::json::ParseOptions nesting;
    nesting.max_nesting = 2;
    const auto nesting_result = d2t::json::parse("[[[]]]", nesting);
    check(!nesting_result.ok() && nesting_result.error && nesting_result.error->code == "JSON_NESTING_LIMIT", "nesting limit");

    d2t::json::ParseOptions input_limit;
    input_limit.max_bytes = 4;
    const auto input_limit_result = d2t::json::parse("[123]", input_limit);
    check(!input_limit_result.ok() && input_limit_result.error && input_limit_result.error->code == "JSON_INPUT_TOO_LARGE", "input byte limit");

    d2t::json::ParseOptions string_limit;
    string_limit.max_string_bytes = 3;
    const auto string_limit_result = d2t::json::parse("\"abcd\"", string_limit);
    check(!string_limit_result.ok() && string_limit_result.error && string_limit_result.error->code == "JSON_STRING_TOO_LARGE", "string byte limit");

    d2t::json::ParseOptions exact_limit;
    exact_limit.max_bytes = 5;
    exact_limit.max_string_bytes = 3;
    check(d2t::json::parse("\"abc\"", exact_limit).ok(), "exact resource limits accepted");

    const auto object = d2t::json::parse("{\"name\":\"alpha\"}");
    check(object.ok(), "accessor parse");
    if (object.ok()) {
        const auto* name = d2t::json::find_member(*object.value, "name");
        check(name != nullptr && name->is_string() && name->as_string() == "alpha", "find member");
        check(d2t::json::find_member(*object.value, "missing") == nullptr, "missing member");
    }

    if (failures == 0) {
        std::cout << "json_tests: all checks passed\n";
        return 0;
    }
    std::cerr << "json_tests: " << failures << " failure(s)\n";
    return 1;
}
