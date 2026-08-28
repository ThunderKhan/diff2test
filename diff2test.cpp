#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace d2t::core {

enum class Outcome {
    SubsetSelected = 0,
    FullSuiteSelected = 10,
    FullSuiteRequired = 11,
    UsageError = 64,
    InputError = 65,
    InternalError = 70,
};

constexpr std::string_view kVersion = "0.1.0-dev";

int exit_code(Outcome outcome) {
    return static_cast<int>(outcome);
}

}  // namespace d2t::core

namespace d2t::json {

struct Position {
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Error {
    Position position;
    std::string code;
    std::string message;
};

struct Null final {};
struct Value;
using Array = std::vector<Value>;
using Object = std::map<std::string, Value, std::less<>>;

struct Value {
    using Storage = std::variant<Null, bool, double, std::string, Array, Object>;
    Storage data;

    Value() : data(Null{}) {}
    explicit Value(Null value) : data(value) {}
    explicit Value(bool value) : data(value) {}
    explicit Value(double value) : data(value) {}
    explicit Value(std::string value) : data(std::move(value)) {}
    explicit Value(Array value) : data(std::move(value)) {}
    explicit Value(Object value) : data(std::move(value)) {}

    bool is_null() const { return std::holds_alternative<Null>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    bool is_number() const { return std::holds_alternative<double>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_array() const { return std::holds_alternative<Array>(data); }
    bool is_object() const { return std::holds_alternative<Object>(data); }

    const bool& as_bool() const { return std::get<bool>(data); }
    const double& as_number() const { return std::get<double>(data); }
    const std::string& as_string() const { return std::get<std::string>(data); }
    const Array& as_array() const { return std::get<Array>(data); }
    const Object& as_object() const { return std::get<Object>(data); }
};

struct ParseOptions {
    std::size_t max_bytes = 64U * 1024U * 1024U;
    std::size_t max_nesting = 256U;
    std::size_t max_string_bytes = 16U * 1024U * 1024U;
};

struct ParseResult {
    std::optional<Value> value;
    std::optional<Error> error;

    bool ok() const { return value.has_value(); }
};

namespace detail {

bool is_continuation_byte(unsigned char c) {
    return (c & 0xC0U) == 0x80U;
}

bool is_valid_utf8(std::string_view input) {
    std::size_t i = 0;
    while (i < input.size()) {
        const unsigned char lead = static_cast<unsigned char>(input[i]);
        if (lead <= 0x7FU) {
            ++i;
            continue;
        }

        std::size_t length = 0;
        std::uint32_t codepoint = 0;
        if ((lead & 0xE0U) == 0xC0U) {
            length = 2;
            codepoint = static_cast<std::uint32_t>(lead & 0x1FU);
            if (lead < 0xC2U) return false;
        } else if ((lead & 0xF0U) == 0xE0U) {
            length = 3;
            codepoint = static_cast<std::uint32_t>(lead & 0x0FU);
        } else if ((lead & 0xF8U) == 0xF0U) {
            length = 4;
            codepoint = static_cast<std::uint32_t>(lead & 0x07U);
            if (lead > 0xF4U) return false;
        } else {
            return false;
        }

        if (i + length > input.size()) return false;
        for (std::size_t j = 1; j < length; ++j) {
            const unsigned char next = static_cast<unsigned char>(input[i + j]);
            if (!is_continuation_byte(next)) return false;
            codepoint = (codepoint << 6U) | static_cast<std::uint32_t>(next & 0x3FU);
        }

        if ((length == 3 && codepoint < 0x800U) ||
            (length == 4 && codepoint < 0x10000U) ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU) ||
            codepoint > 0x10FFFFU) {
            return false;
        }
        i += length;
    }
    return true;
}

void append_utf8(std::string& out, std::uint32_t codepoint) {
    if (codepoint <= 0x7FU) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFU) {
        out.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else if (codepoint <= 0xFFFFU) {
        out.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else {
        out.push_back(static_cast<char>(0xF0U | (codepoint >> 18U)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    }
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

class Parser {
public:
    Parser(std::string_view input, ParseOptions options)
        : input_(input), options_(options) {}

    ParseResult parse() {
        if (input_.size() > options_.max_bytes) {
            return fail("JSON_INPUT_TOO_LARGE", "JSON input exceeds configured size limit");
        }
        if (!is_valid_utf8(input_)) {
            return fail("JSON_INVALID_UTF8", "JSON input is not valid UTF-8");
        }

        skip_ws();
        auto value = parse_value(0);
        if (!value.has_value()) return {std::nullopt, error_};
        skip_ws();
        if (!at_end()) {
            return fail("JSON_TRAILING_DATA", "unexpected non-whitespace after JSON value");
        }
        return {std::move(value), std::nullopt};
    }

private:
    std::string_view input_;
    ParseOptions options_;
    Position position_{};
    std::optional<Error> error_;

    bool at_end() const { return position_.offset >= input_.size(); }
    char peek() const { return at_end() ? '\0' : input_[position_.offset]; }

    char advance() {
        const char c = input_[position_.offset++];
        if (c == '\n') {
            ++position_.line;
            position_.column = 1;
        } else {
            ++position_.column;
        }
        return c;
    }

    void skip_ws() {
        while (!at_end()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else {
                break;
            }
        }
    }

    ParseResult fail(std::string code, std::string message) {
        Error error{position_, std::move(code), std::move(message)};
        error_ = error;
        return {std::nullopt, std::move(error)};
    }

    std::optional<Value> set_error(std::string code, std::string message) {
        error_ = Error{position_, std::move(code), std::move(message)};
        return std::nullopt;
    }

    bool consume_literal(std::string_view literal) {
        for (char expected : literal) {
            if (at_end() || peek() != expected) return false;
            advance();
        }
        return true;
    }

    std::optional<Value> parse_value(std::size_t depth) {
        if (depth > options_.max_nesting) {
            return set_error("JSON_NESTING_LIMIT", "JSON nesting exceeds configured limit");
        }
        if (at_end()) return set_error("JSON_EXPECTED_VALUE", "expected JSON value");

        switch (peek()) {
            case 'n':
                if (!consume_literal("null")) return set_error("JSON_INVALID_LITERAL", "invalid literal; expected null");
                return Value{Null{}};
            case 't':
                if (!consume_literal("true")) return set_error("JSON_INVALID_LITERAL", "invalid literal; expected true");
                return Value{true};
            case 'f':
                if (!consume_literal("false")) return set_error("JSON_INVALID_LITERAL", "invalid literal; expected false");
                return Value{false};
            case '"': {
                auto string = parse_string();
                if (!string.has_value()) return std::nullopt;
                return Value{std::move(*string)};
            }
            case '[':
                return parse_array(depth + 1U);
            case '{':
                return parse_object(depth + 1U);
            default:
                if (peek() == '-' || (peek() >= '0' && peek() <= '9')) return parse_number();
                return set_error("JSON_EXPECTED_VALUE", "unexpected character where JSON value was expected");
        }
    }

    std::optional<std::uint16_t> parse_hex4() {
        if (input_.size() - position_.offset < 4U) {
            set_error("JSON_INVALID_UNICODE_ESCAPE", "incomplete \\u escape");
            return std::nullopt;
        }
        std::uint16_t value = 0;
        for (int i = 0; i < 4; ++i) {
            const int digit = hex_value(peek());
            if (digit < 0) {
                set_error("JSON_INVALID_UNICODE_ESCAPE", "non-hex digit in \\u escape");
                return std::nullopt;
            }
            advance();
            value = static_cast<std::uint16_t>((value << 4U) | static_cast<std::uint16_t>(digit));
        }
        return value;
    }

    std::optional<std::string> parse_string() {
        advance();
        std::string out;
        while (!at_end()) {
            const unsigned char c = static_cast<unsigned char>(peek());
            if (c == '"') {
                advance();
                return out;
            }
            if (c < 0x20U) {
                set_error("JSON_CONTROL_IN_STRING", "unescaped control character in string");
                return std::nullopt;
            }
            if (c != '\\') {
                out.push_back(advance());
                if (out.size() > options_.max_string_bytes) {
                    set_error("JSON_STRING_TOO_LARGE", "JSON string exceeds configured size limit");
                    return std::nullopt;
                }
                continue;
            }

            advance();
            if (at_end()) {
                set_error("JSON_UNTERMINATED_STRING", "unterminated escape sequence in string");
                return std::nullopt;
            }
            const char escaped = advance();
            switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    auto first = parse_hex4();
                    if (!first.has_value()) return std::nullopt;
                    std::uint32_t codepoint = *first;
                    if (codepoint >= 0xD800U && codepoint <= 0xDBFFU) {
                        if (input_.size() - position_.offset < 6U || peek() != '\\' || input_[position_.offset + 1U] != 'u') {
                            set_error("JSON_UNPAIRED_SURROGATE", "high surrogate must be followed by a low surrogate");
                            return std::nullopt;
                        }
                        advance();
                        advance();
                        auto second = parse_hex4();
                        if (!second.has_value()) return std::nullopt;
                        if (*second < 0xDC00U || *second > 0xDFFFU) {
                            set_error("JSON_UNPAIRED_SURROGATE", "high surrogate is not followed by a low surrogate");
                            return std::nullopt;
                        }
                        codepoint = 0x10000U + ((codepoint - 0xD800U) << 10U) + (static_cast<std::uint32_t>(*second) - 0xDC00U);
                    } else if (codepoint >= 0xDC00U && codepoint <= 0xDFFFU) {
                        set_error("JSON_UNPAIRED_SURROGATE", "low surrogate appears without a preceding high surrogate");
                        return std::nullopt;
                    }
                    append_utf8(out, codepoint);
                    break;
                }
                default:
                    set_error("JSON_INVALID_ESCAPE", "invalid escape sequence in string");
                    return std::nullopt;
            }
            if (out.size() > options_.max_string_bytes) {
                set_error("JSON_STRING_TOO_LARGE", "JSON string exceeds configured size limit");
                return std::nullopt;
            }
        }
        set_error("JSON_UNTERMINATED_STRING", "unterminated JSON string");
        return std::nullopt;
    }

    std::optional<Value> parse_number() {
        const std::size_t start = position_.offset;
        if (peek() == '-') advance();
        if (at_end()) return set_error("JSON_INVALID_NUMBER", "expected digit after minus sign");

        if (peek() == '0') {
            advance();
            if (!at_end() && peek() >= '0' && peek() <= '9') {
                return set_error("JSON_INVALID_NUMBER", "leading zero is not allowed in JSON number");
            }
        } else if (peek() >= '1' && peek() <= '9') {
            while (!at_end() && peek() >= '0' && peek() <= '9') advance();
        } else {
            return set_error("JSON_INVALID_NUMBER", "expected digit in JSON number");
        }

        if (!at_end() && peek() == '.') {
            advance();
            if (at_end() || peek() < '0' || peek() > '9') {
                return set_error("JSON_INVALID_NUMBER", "fraction requires at least one digit");
            }
            while (!at_end() && peek() >= '0' && peek() <= '9') advance();
        }

        if (!at_end() && (peek() == 'e' || peek() == 'E')) {
            advance();
            if (!at_end() && (peek() == '+' || peek() == '-')) advance();
            if (at_end() || peek() < '0' || peek() > '9') {
                return set_error("JSON_INVALID_NUMBER", "exponent requires at least one digit");
            }
            while (!at_end() && peek() >= '0' && peek() <= '9') advance();
        }

        const std::string_view token = input_.substr(start, position_.offset - start);
        double value = 0.0;
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value, std::chars_format::general);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size() || !std::isfinite(value)) {
            return set_error("JSON_NUMBER_RANGE", "JSON number is outside supported finite double range");
        }
        return Value{value};
    }

    std::optional<Value> parse_array(std::size_t depth) {
        if (depth > options_.max_nesting) return set_error("JSON_NESTING_LIMIT", "JSON nesting exceeds configured limit");
        advance();
        skip_ws();
        Array values;
        if (!at_end() && peek() == ']') {
            advance();
            return Value{std::move(values)};
        }

        while (true) {
            skip_ws();
            auto value = parse_value(depth);
            if (!value.has_value()) return std::nullopt;
            values.push_back(std::move(*value));
            skip_ws();
            if (at_end()) return set_error("JSON_UNTERMINATED_ARRAY", "unterminated JSON array");
            if (peek() == ']') {
                advance();
                return Value{std::move(values)};
            }
            if (peek() != ',') return set_error("JSON_EXPECTED_COMMA", "expected ',' or ']' in array");
            advance();
            skip_ws();
            if (!at_end() && peek() == ']') return set_error("JSON_TRAILING_COMMA", "trailing comma is not allowed in array");
        }
    }

    std::optional<Value> parse_object(std::size_t depth) {
        if (depth > options_.max_nesting) return set_error("JSON_NESTING_LIMIT", "JSON nesting exceeds configured limit");
        advance();
        skip_ws();
        Object object;
        if (!at_end() && peek() == '}') {
            advance();
            return Value{std::move(object)};
        }

        while (true) {
            skip_ws();
            if (at_end() || peek() != '"') return set_error("JSON_EXPECTED_KEY", "expected string key in object");
            auto key = parse_string();
            if (!key.has_value()) return std::nullopt;
            skip_ws();
            if (at_end() || peek() != ':') return set_error("JSON_EXPECTED_COLON", "expected ':' after object key");
            advance();
            skip_ws();
            auto value = parse_value(depth);
            if (!value.has_value()) return std::nullopt;
            auto [it, inserted] = object.emplace(std::move(*key), std::move(*value));
            (void)it;
            if (!inserted) return set_error("JSON_DUPLICATE_KEY", "duplicate object member is not accepted");
            skip_ws();
            if (at_end()) return set_error("JSON_UNTERMINATED_OBJECT", "unterminated JSON object");
            if (peek() == '}') {
                advance();
                return Value{std::move(object)};
            }
            if (peek() != ',') return set_error("JSON_EXPECTED_COMMA", "expected ',' or '}' in object");
            advance();
            skip_ws();
            if (!at_end() && peek() == '}') return set_error("JSON_TRAILING_COMMA", "trailing comma is not allowed in object");
        }
    }
};

}  // namespace detail

ParseResult parse(std::string_view input, ParseOptions options = {}) {
    return detail::Parser(input, options).parse();
}

const Value* find_member(const Value& value, std::string_view key) {
    if (!value.is_object()) return nullptr;
    const auto& object = value.as_object();
    const auto it = object.find(key);
    return it == object.end() ? nullptr : &it->second;
}

}  // namespace d2t::json

namespace d2t::dep {

struct Rule {
    std::vector<std::string> targets;
    std::vector<std::string> prerequisites;
};

struct Error {
    std::size_t line = 1;
    std::size_t column = 1;
    std::string code;
    std::string message;
};

struct ParseResult {
    std::vector<Rule> rules;
    std::optional<Error> error;

    bool ok() const { return !error.has_value(); }
};

namespace detail {

struct LogicalLine {
    std::string text;
    std::size_t source_line = 1;
};

std::optional<std::vector<LogicalLine>> fold_lines(std::string_view input, Error& error) {
    std::vector<LogicalLine> lines;
    std::string current;
    std::size_t logical_start = 1;
    std::size_t line = 1;
    std::size_t i = 0;

    while (i < input.size()) {
        const char c = input[i];
        if (c == '\0') {
            error = Error{line, 1, "DEP_NUL", "dependency input contains NUL"};
            return std::nullopt;
        }
        if (c == '\\') {
            if (i + 1U < input.size() && input[i + 1U] == '\n') {
                current.push_back(' ');
                i += 2U;
                ++line;
                continue;
            }
            if (i + 2U < input.size() && input[i + 1U] == '\r' && input[i + 2U] == '\n') {
                current.push_back(' ');
                i += 3U;
                ++line;
                continue;
            }
        }
        if (c == '\r' && i + 1U < input.size() && input[i + 1U] == '\n') {
            lines.push_back(LogicalLine{std::move(current), logical_start});
            current.clear();
            i += 2U;
            ++line;
            logical_start = line;
            continue;
        }
        if (c == '\n') {
            lines.push_back(LogicalLine{std::move(current), logical_start});
            current.clear();
            ++i;
            ++line;
            logical_start = line;
            continue;
        }
        current.push_back(c);
        ++i;
    }

    if (!current.empty()) lines.push_back(LogicalLine{std::move(current), logical_start});
    return lines;
}

std::optional<std::size_t> find_rule_colon(std::string_view line) {
    bool escaped = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '#') return std::nullopt;
        if (c == ':') return i;
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> tokenize(std::string_view text, std::size_t source_line, Error& error) {
    std::vector<std::string> tokens;
    std::string token;
    bool escaped = false;

    auto flush = [&]() {
        if (!token.empty()) {
            tokens.push_back(std::move(token));
            token.clear();
        }
    };

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (escaped) {
            token.push_back(c);
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '#') break;
        if (c == ' ' || c == '\t') {
            flush();
            continue;
        }
        token.push_back(c);
    }

    if (escaped) {
        error = Error{source_line, text.size() + 1U, "DEP_TRAILING_ESCAPE", "dependency rule ends with an escape"};
        return std::nullopt;
    }
    flush();
    return tokens;
}

}  // namespace detail

ParseResult parse(std::string_view input) {
    ParseResult result;
    Error error;
    auto lines = detail::fold_lines(input, error);
    if (!lines.has_value()) {
        result.error = std::move(error);
        return result;
    }

    for (const auto& logical : *lines) {
        const auto first_non_space = logical.text.find_first_not_of(" \t");
        if (first_non_space == std::string::npos || logical.text[first_non_space] == '#') continue;

        const auto colon = detail::find_rule_colon(logical.text);
        if (!colon.has_value()) {
            result.error = Error{logical.source_line, 1, "DEP_MISSING_COLON", "dependency rule has no unescaped ':' separator"};
            return result;
        }

        auto targets = detail::tokenize(std::string_view(logical.text).substr(0, *colon), logical.source_line, error);
        if (!targets.has_value()) {
            result.error = std::move(error);
            return result;
        }
        if (targets->empty()) {
            result.error = Error{logical.source_line, *colon + 1U, "DEP_EMPTY_TARGET", "dependency rule has no target"};
            return result;
        }

        auto prerequisites = detail::tokenize(std::string_view(logical.text).substr(*colon + 1U), logical.source_line, error);
        if (!prerequisites.has_value()) {
            result.error = std::move(error);
            return result;
        }

        std::sort(prerequisites->begin(), prerequisites->end());
        prerequisites->erase(std::unique(prerequisites->begin(), prerequisites->end()), prerequisites->end());
        result.rules.push_back(Rule{std::move(*targets), std::move(*prerequisites)});
    }

    return result;
}

}  // namespace d2t::dep

namespace d2t::path {

enum class RootClass {
    Project,
    Build,
    External,
    Invalid,
};

struct NormalizedPath {
    std::filesystem::path absolute;
    std::filesystem::path display;
    RootClass root_class = RootClass::Invalid;
};

struct Result {
    std::optional<NormalizedPath> value;
    std::string error;

    bool ok() const { return value.has_value(); }
};

bool has_prefix(const std::filesystem::path& path, const std::filesystem::path& root) {
    auto path_it = path.begin();
    auto root_it = root.begin();
    for (; root_it != root.end(); ++root_it, ++path_it) {
        if (path_it == path.end() || *path_it != *root_it) return false;
    }
    return true;
}

Result normalize(std::string_view raw,
                 const std::filesystem::path& project_root,
                 const std::filesystem::path& build_root,
                 bool allow_external = false) {
    if (raw.empty()) return {std::nullopt, "path is empty"};
    if (raw.find('\0') != std::string_view::npos) return {std::nullopt, "path contains NUL"};

    const auto project = project_root.lexically_normal();
    const auto build = build_root.lexically_normal();
    if (!project.is_absolute() || !build.is_absolute()) {
        return {std::nullopt, "project and build roots must be absolute"};
    }

    std::filesystem::path candidate{std::string(raw)};
    if (!candidate.is_absolute()) candidate = project / candidate;
    candidate = candidate.lexically_normal();

    if (has_prefix(candidate, project)) {
        return {NormalizedPath{candidate, candidate.lexically_relative(project), RootClass::Project}, {}};
    }
    if (has_prefix(candidate, build)) {
        return {NormalizedPath{candidate, candidate.lexically_relative(build), RootClass::Build}, {}};
    }
    if (allow_external && candidate.is_absolute()) {
        return {NormalizedPath{candidate, candidate, RootClass::External}, {}};
    }
    return {std::nullopt, "path escapes declared project/build roots"};
}

}  // namespace d2t::path

namespace d2t::cli {

struct AnalyzeOptions {
    std::filesystem::path project_root;
    std::filesystem::path build_root;
    std::string changed_files;
    std::filesystem::path cmake_reply;
    std::optional<std::filesystem::path> cmake_index;
    std::filesystem::path ctest_info;
    std::optional<std::filesystem::path> dep_list;
    std::optional<std::string> configuration;
    std::string format = "human";
    bool explain = false;
    bool verbose = false;
};

struct ParseResult {
    bool ok = false;
    AnalyzeOptions options;
    std::string error;
};

void print_help(std::ostream& out) {
    out <<
        "diff2test - conservative C++ test-impact analysis\n\n"
        "Usage:\n"
        "  diff2test --help\n"
        "  diff2test --version\n"
        "  diff2test analyze [options]\n\n"
        "Required analyze options:\n"
        "  --project-root <dir>\n"
        "  --build-root <dir>\n"
        "  --changed-files <file|->\n"
        "  --cmake-reply <dir>\n"
        "  --ctest-info <file>\n"
        "  --dep-list <file>\n\n"
        "Optional:\n"
        "  --cmake-index <file>\n"
        "  --configuration <name>\n"
        "  --format <human|names>\n"
        "  --explain\n"
        "  --verbose\n\n"
        "diff2test only reads pre-generated metadata. It never launches Git, CMake,\n"
        "CTest, a compiler, a shell, or any other external program.\n\n"
        "Safety statuses:\n"
        "  0   SUBSET_SELECTED\n"
        "  10  FULL_SUITE_SELECTED\n"
        "  11  FULL_SUITE_REQUIRED\n"
        "  64  USAGE_ERROR\n"
        "  65  INPUT_ERROR\n"
        "  70  INTERNAL_ERROR\n";
}

ParseResult parse_analyze(int argc, char** argv) {
    ParseResult result;
    std::map<std::string, std::string> values;
    std::set<std::string> flags;

    const std::set<std::string> value_options = {
        "--project-root", "--build-root", "--changed-files", "--cmake-reply",
        "--cmake-index", "--ctest-info", "--dep-list", "--configuration", "--format"
    };
    const std::set<std::string> flag_options = {"--explain", "--verbose"};

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (value_options.contains(arg)) {
            if (values.contains(arg)) {
                result.error = "repeated option: " + arg;
                return result;
            }
            if (i + 1 >= argc) {
                result.error = "missing value for option: " + arg;
                return result;
            }
            values.emplace(arg, argv[++i]);
            continue;
        }
        if (flag_options.contains(arg)) {
            if (!flags.insert(arg).second) {
                result.error = "repeated option: " + arg;
                return result;
            }
            continue;
        }
        result.error = "unknown option: " + arg;
        return result;
    }

    const std::vector<std::string> required = {
        "--project-root", "--build-root", "--changed-files", "--cmake-reply", "--ctest-info", "--dep-list"
    };
    for (const auto& option : required) {
        if (!values.contains(option)) {
            result.error = "missing required option: " + option;
            return result;
        }
    }

    if (values.contains("--format") && values["--format"] != "human" && values["--format"] != "names") {
        result.error = "--format must be either 'human' or 'names'";
        return result;
    }

    result.options.project_root = values["--project-root"];
    result.options.build_root = values["--build-root"];
    result.options.changed_files = values["--changed-files"];
    result.options.cmake_reply = values["--cmake-reply"];
    result.options.ctest_info = values["--ctest-info"];
    result.options.dep_list = values["--dep-list"];
    if (values.contains("--cmake-index")) result.options.cmake_index = values["--cmake-index"];
    if (values.contains("--configuration")) result.options.configuration = values["--configuration"];
    if (values.contains("--format")) result.options.format = values["--format"];
    result.options.explain = flags.contains("--explain");
    result.options.verbose = flags.contains("--verbose");
    result.ok = true;
    return result;
}

}  // namespace d2t::cli

#ifndef D2T_TESTING
namespace d2t {

int run(int argc, char** argv) {
    using core::Outcome;

    if (argc == 2) {
        const std::string_view arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            cli::print_help(std::cout);
            return core::exit_code(Outcome::SubsetSelected);
        }
        if (arg == "--version") {
            std::cout << "diff2test " << core::kVersion << '\n';
            return core::exit_code(Outcome::SubsetSelected);
        }
    }

    if (argc < 2 || std::string_view(argv[1]) != "analyze") {
        std::cerr << "diff2test: expected command 'analyze' (try --help)\n";
        return core::exit_code(Outcome::UsageError);
    }

    const auto parsed = cli::parse_analyze(argc, argv);
    if (!parsed.ok) {
        std::cerr << "diff2test: " << parsed.error << '\n';
        return core::exit_code(Outcome::UsageError);
    }

    std::cerr << "diff2test: analysis pipeline not implemented yet\n";
    return core::exit_code(Outcome::InputError);
}

}  // namespace d2t

int main(int argc, char** argv) {
    try {
        return d2t::run(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "diff2test: internal error: " << ex.what() << '\n';
        return d2t::core::exit_code(d2t::core::Outcome::InternalError);
    } catch (...) {
        std::cerr << "diff2test: internal error: unknown exception\n";
        return d2t::core::exit_code(d2t::core::Outcome::InternalError);
    }
}
#endif
