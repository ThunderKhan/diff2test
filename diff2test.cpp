#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
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
  InternalError = 70
};
constexpr std::string_view kVersion = "0.1.0";
int exit_code(Outcome o) { return static_cast<int>(o); }
} // namespace d2t::core

namespace d2t::json {
struct Position {
  std::size_t offset = 0, line = 1, column = 1;
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
  explicit Value(Null v) : data(v) {}
  explicit Value(bool v) : data(v) {}
  explicit Value(double v) : data(v) {}
  explicit Value(std::string v) : data(std::move(v)) {}
  explicit Value(Array v) : data(std::move(v)) {}
  explicit Value(Object v) : data(std::move(v)) {}
  bool is_null() const { return std::holds_alternative<Null>(data); }
  bool is_bool() const { return std::holds_alternative<bool>(data); }
  bool is_number() const { return std::holds_alternative<double>(data); }
  bool is_string() const { return std::holds_alternative<std::string>(data); }
  bool is_array() const { return std::holds_alternative<Array>(data); }
  bool is_object() const { return std::holds_alternative<Object>(data); }
  const bool &as_bool() const { return std::get<bool>(data); }
  const double &as_number() const { return std::get<double>(data); }
  const std::string &as_string() const { return std::get<std::string>(data); }
  const Array &as_array() const { return std::get<Array>(data); }
  const Object &as_object() const { return std::get<Object>(data); }
};
struct ParseOptions {
  std::size_t max_bytes = 64U * 1024U * 1024U, max_nesting = 256U,
              max_string_bytes = 16U * 1024U * 1024U;
};
struct ParseResult {
  std::optional<Value> value;
  std::optional<Error> error;
  bool ok() const { return value.has_value(); }
};
namespace detail {
bool cont(unsigned char c) { return (c & 0xC0U) == 0x80U; }
bool valid_utf8(std::string_view s) {
  for (std::size_t i = 0; i < s.size();) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (c <= 0x7F) {
      ++i;
      continue;
    }
    std::size_t n = 0;
    std::uint32_t cp = 0;
    if ((c & 0xE0U) == 0xC0U) {
      if (c < 0xC2U)
        return false;
      n = 2;
      cp = c & 0x1FU;
    } else if ((c & 0xF0U) == 0xE0U) {
      n = 3;
      cp = c & 0x0FU;
    } else if ((c & 0xF8U) == 0xF0U) {
      if (c > 0xF4U)
        return false;
      n = 4;
      cp = c & 0x07U;
    } else
      return false;
    if (i + n > s.size())
      return false;
    for (std::size_t j = 1; j < n; ++j) {
      unsigned char d = static_cast<unsigned char>(s[i + j]);
      if (!cont(d))
        return false;
      cp = (cp << 6U) | (d & 0x3FU);
    }
    if ((n == 3 && cp < 0x800U) || (n == 4 && cp < 0x10000U) ||
        (cp >= 0xD800U && cp <= 0xDFFFU) || cp > 0x10FFFFU)
      return false;
    i += n;
  }
  return true;
}
void append_utf8(std::string &out, std::uint32_t cp) {
  if (cp <= 0x7F)
    out.push_back(static_cast<char>(cp));
  else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0U | (cp >> 6U)));
    out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
  } else if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0U | (cp >> 12U)));
    out.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
  } else {
    out.push_back(static_cast<char>(0xF0U | (cp >> 18U)));
    out.push_back(static_cast<char>(0x80U | ((cp >> 12U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
  }
}
int hex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}
class Parser {
  std::string_view in;
  ParseOptions opt;
  Position pos{};
  std::optional<Error> err;
  bool end() const { return pos.offset >= in.size(); }
  char peek() const { return end() ? '\0' : in[pos.offset]; }
  char adv() {
    char c = in[pos.offset++];
    if (c == '\n') {
      ++pos.line;
      pos.column = 1;
    } else
      ++pos.column;
    return c;
  }
  void ws() {
    while (!end() && (peek() == ' ' || peek() == '\t' || peek() == '\r' ||
                      peek() == '\n'))
      adv();
  }
  std::optional<Value> failv(std::string c, std::string m) {
    err = Error{pos, std::move(c), std::move(m)};
    return std::nullopt;
  }
  bool literal(std::string_view x) {
    for (char c : x) {
      if (end() || peek() != c)
        return false;
      adv();
    }
    return true;
  }
  std::optional<std::uint16_t> hex4() {
    if (in.size() - pos.offset < 4) {
      failv("JSON_INVALID_UNICODE_ESCAPE", "incomplete unicode escape");
      return std::nullopt;
    }
    std::uint16_t v = 0;
    for (int i = 0; i < 4; ++i) {
      int d = hex(peek());
      if (d < 0) {
        failv("JSON_INVALID_UNICODE_ESCAPE", "invalid unicode escape");
        return std::nullopt;
      }
      adv();
      v = static_cast<std::uint16_t>((v << 4U) | static_cast<unsigned>(d));
    }
    return v;
  }
  std::optional<std::string> str() {
    adv();
    std::string out;
    while (!end()) {
      unsigned char c = static_cast<unsigned char>(peek());
      if (c == '"') {
        adv();
        return out;
      }
      if (c < 0x20U) {
        failv("JSON_CONTROL_IN_STRING", "control character in string");
        return std::nullopt;
      }
      if (c != '\\') {
        out.push_back(adv());
        if (out.size() > opt.max_string_bytes) {
          failv("JSON_STRING_TOO_LARGE", "string limit");
          return std::nullopt;
        }
        continue;
      }
      adv();
      if (end()) {
        failv("JSON_UNTERMINATED_STRING", "unterminated string");
        return std::nullopt;
      }
      char e = adv();
      switch (e) {
      case '"':
        out.push_back('"');
        break;
      case '\\':
        out.push_back('\\');
        break;
      case '/':
        out.push_back('/');
        break;
      case 'b':
        out.push_back('\b');
        break;
      case 'f':
        out.push_back('\f');
        break;
      case 'n':
        out.push_back('\n');
        break;
      case 'r':
        out.push_back('\r');
        break;
      case 't':
        out.push_back('\t');
        break;
      case 'u': {
        auto a = hex4();
        if (!a)
          return std::nullopt;
        std::uint32_t cp = *a;
        if (cp >= 0xD800U && cp <= 0xDBFFU) {
          if (in.size() - pos.offset < 6 || peek() != '\\' ||
              in[pos.offset + 1] != 'u') {
            failv("JSON_UNPAIRED_SURROGATE", "unpaired surrogate");
            return std::nullopt;
          }
          adv();
          adv();
          auto b = hex4();
          if (!b)
            return std::nullopt;
          if (*b < 0xDC00U || *b > 0xDFFFU) {
            failv("JSON_UNPAIRED_SURROGATE", "unpaired surrogate");
            return std::nullopt;
          }
          cp = 0x10000U + ((cp - 0xD800U) << 10U) +
               (static_cast<std::uint32_t>(*b) - 0xDC00U);
        } else if (cp >= 0xDC00U && cp <= 0xDFFFU) {
          failv("JSON_UNPAIRED_SURROGATE", "unpaired surrogate");
          return std::nullopt;
        }
        append_utf8(out, cp);
        break;
      }
      default:
        failv("JSON_INVALID_ESCAPE", "invalid escape");
        return std::nullopt;
      }
      if (out.size() > opt.max_string_bytes) {
        failv("JSON_STRING_TOO_LARGE", "string limit");
        return std::nullopt;
      }
    }
    failv("JSON_UNTERMINATED_STRING", "unterminated string");
    return std::nullopt;
  }
  std::optional<Value> num() {
    std::size_t start = pos.offset;
    if (peek() == '-')
      adv();
    if (end())
      return failv("JSON_INVALID_NUMBER", "bad number");
    if (peek() == '0') {
      adv();
      if (!end() && std::isdigit(static_cast<unsigned char>(peek())))
        return failv("JSON_INVALID_NUMBER", "leading zero");
    } else if (peek() >= '1' && peek() <= '9') {
      while (!end() && std::isdigit(static_cast<unsigned char>(peek())))
        adv();
    } else
      return failv("JSON_INVALID_NUMBER", "bad number");
    if (!end() && peek() == '.') {
      adv();
      if (end() || !std::isdigit(static_cast<unsigned char>(peek())))
        return failv("JSON_INVALID_NUMBER", "bad fraction");
      while (!end() && std::isdigit(static_cast<unsigned char>(peek())))
        adv();
    }
    if (!end() && (peek() == 'e' || peek() == 'E')) {
      adv();
      if (!end() && (peek() == '+' || peek() == '-'))
        adv();
      if (end() || !std::isdigit(static_cast<unsigned char>(peek())))
        return failv("JSON_INVALID_NUMBER", "bad exponent");
      while (!end() && std::isdigit(static_cast<unsigned char>(peek())))
        adv();
    }
    std::string tmp(in.substr(start, pos.offset - start));
    char *ep = nullptr;
    double v = std::strtod(tmp.c_str(), &ep);
    if (ep != tmp.c_str() + tmp.size() || !std::isfinite(v))
      return failv("JSON_INVALID_NUMBER", "number out of range");
    return Value{v};
  }
  std::optional<Value> arr(std::size_t depth) {
    if (depth > opt.max_nesting)
      return failv("JSON_NESTING_LIMIT", "nesting limit");
    adv();
    ws();
    Array a;
    if (!end() && peek() == ']') {
      adv();
      return Value{std::move(a)};
    }
    while (true) {
      auto v = value(depth);
      if (!v)
        return std::nullopt;
      a.push_back(std::move(*v));
      ws();
      if (end())
        return failv("JSON_EXPECTED_COMMA", "unterminated array");
      if (peek() == ']') {
        adv();
        break;
      }
      if (peek() != ',')
        return failv("JSON_EXPECTED_COMMA", "expected comma");
      adv();
      ws();
      if (!end() && peek() == ']')
        return failv("JSON_TRAILING_COMMA", "trailing comma");
    }
    return Value{std::move(a)};
  }
  std::optional<Value> obj(std::size_t depth) {
    if (depth > opt.max_nesting)
      return failv("JSON_NESTING_LIMIT", "nesting limit");
    adv();
    ws();
    Object o;
    if (!end() && peek() == '}') {
      adv();
      return Value{std::move(o)};
    }
    while (true) {
      if (end() || peek() != '"')
        return failv("JSON_EXPECTED_KEY", "expected key");
      auto k = str();
      if (!k)
        return std::nullopt;
      ws();
      if (end() || peek() != ':')
        return failv("JSON_EXPECTED_COLON", "expected colon");
      adv();
      ws();
      auto v = value(depth);
      if (!v)
        return std::nullopt;
      if (o.contains(*k))
        return failv("JSON_DUPLICATE_KEY", "duplicate key");
      o.emplace(std::move(*k), std::move(*v));
      ws();
      if (end())
        return failv("JSON_EXPECTED_COMMA", "unterminated object");
      if (peek() == '}') {
        adv();
        break;
      }
      if (peek() != ',')
        return failv("JSON_EXPECTED_COMMA", "expected comma");
      adv();
      ws();
      if (!end() && peek() == '}')
        return failv("JSON_TRAILING_COMMA", "trailing comma");
    }
    return Value{std::move(o)};
  }
  std::optional<Value> value(std::size_t depth) {
    if (depth > opt.max_nesting)
      return failv("JSON_NESTING_LIMIT", "nesting limit");
    if (end())
      return failv("JSON_EXPECTED_VALUE", "expected value");
    if (peek() == 'n') {
      if (!literal("null"))
        return failv("JSON_INVALID_LITERAL", "bad literal");
      return Value{Null{}};
    }
    if (peek() == 't') {
      if (!literal("true"))
        return failv("JSON_INVALID_LITERAL", "bad literal");
      return Value{true};
    }
    if (peek() == 'f') {
      if (!literal("false"))
        return failv("JSON_INVALID_LITERAL", "bad literal");
      return Value{false};
    }
    if (peek() == '"') {
      auto x = str();
      return x ? std::optional<Value>{Value{std::move(*x)}} : std::nullopt;
    }
    if (peek() == '[')
      return arr(depth + 1);
    if (peek() == '{')
      return obj(depth + 1);
    if (peek() == '-' || std::isdigit(static_cast<unsigned char>(peek())))
      return num();
    return failv("JSON_EXPECTED_VALUE", "expected value");
  }

public:
  Parser(std::string_view s, ParseOptions o) : in(s), opt(o) {}
  ParseResult parse() {
    if (in.size() > opt.max_bytes)
      return {std::nullopt, Error{pos, "JSON_INPUT_TOO_LARGE", "input limit"}};
    if (!valid_utf8(in))
      return {std::nullopt, Error{pos, "JSON_INVALID_UTF8", "invalid UTF-8"}};
    ws();
    auto v = value(0);
    if (!v)
      return {std::nullopt, err};
    ws();
    if (!end())
      return {std::nullopt, Error{pos, "JSON_TRAILING_DATA", "trailing data"}};
    return {std::move(v), std::nullopt};
  }
};
} // namespace detail
ParseResult parse(std::string_view input, ParseOptions opt = {}) {
  return detail::Parser(input, opt).parse();
}
const Value *find_member(const Value &v, std::string_view key) {
  if (!v.is_object())
    return nullptr;
  auto it = v.as_object().find(key);
  return it == v.as_object().end() ? nullptr : &it->second;
}
} // namespace d2t::json

namespace d2t::dep {
struct Error {
  std::string code, message;
};
struct Rule {
  std::vector<std::string> targets, prerequisites;
};
struct ParseResult {
  std::vector<Rule> rules;
  std::optional<Error> error;
  bool ok() const { return !error.has_value(); }
};
ParseResult parse(std::string_view input) {
  if (input.find('\0') != std::string_view::npos)
    return {{}, Error{"DEP_NUL", "NUL byte"}};
  std::string logical;
  logical.reserve(input.size());
  for (std::size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '\\' && i + 1 < input.size() && input[i + 1] == '\n') {
      ++i;
      logical.push_back(' ');
      continue;
    }
    if (input[i] == '\\' && i + 2 < input.size() && input[i + 1] == '\r' &&
        input[i + 2] == '\n') {
      i += 2;
      logical.push_back(' ');
      continue;
    }
    logical.push_back(input[i]);
  }
  if (!logical.empty() && logical.back() == '\\')
    return {{}, Error{"DEP_TRAILING_ESCAPE", "trailing escape"}};
  ParseResult out;
  std::istringstream lines(logical);
  std::string line;
  while (std::getline(lines, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    bool esc = false;
    std::size_t hash = std::string::npos, colon = std::string::npos;
    for (std::size_t i = 0; i < line.size(); ++i) {
      char c = line[i];
      if (esc) {
        esc = false;
        continue;
      }
      if (c == '\\') {
        esc = true;
        continue;
      }
      if (c == '#') {
        hash = i;
        break;
      }
      if (c == ':' && colon == std::string::npos)
        colon = i;
    }
    if (hash != std::string::npos)
      line.resize(hash);
    auto only_ws = [](std::string_view x) {
      return std::all_of(x.begin(), x.end(),
                         [](char c) { return c == ' ' || c == '\t'; });
    };
    if (line.empty() || only_ws(line))
      continue;
    esc = false;
    colon = std::string::npos;
    for (std::size_t i = 0; i < line.size(); ++i) {
      char c = line[i];
      if (esc) {
        esc = false;
        continue;
      }
      if (c == '\\') {
        esc = true;
        continue;
      }
      if (c == ':') {
        colon = i;
        break;
      }
    }
    if (colon == std::string::npos)
      return {{}, Error{"DEP_MISSING_COLON", "missing rule separator"}};
    auto tokens =
        [](std::string_view s) -> std::optional<std::vector<std::string>> {
      std::vector<std::string> v;
      std::string cur;
      bool escaped = false;
      for (char c : s) {
        if (escaped) {
          cur.push_back(c);
          escaped = false;
          continue;
        }
        if (c == '\\') {
          escaped = true;
          continue;
        }
        if (c == ' ' || c == '\t') {
          if (!cur.empty()) {
            v.push_back(cur);
            cur.clear();
          }
        } else
          cur.push_back(c);
      }
      if (escaped)
        return std::nullopt;
      if (!cur.empty())
        v.push_back(cur);
      return v;
    };
    auto t = tokens(std::string_view(line).substr(0, colon));
    auto p = tokens(std::string_view(line).substr(colon + 1));
    if (!t || !p)
      return {{}, Error{"DEP_TRAILING_ESCAPE", "trailing escape"}};
    if (t->empty())
      return {{}, Error{"DEP_EMPTY_TARGET", "empty target"}};
    std::sort(p->begin(), p->end());
    p->erase(std::unique(p->begin(), p->end()), p->end());
    out.rules.push_back({std::move(*t), std::move(*p)});
  }
  return out;
}
} // namespace d2t::dep

namespace d2t::path {
enum class RootClass { Project, Build, External };
struct Normalized {
  std::filesystem::path absolute, display;
  RootClass root_class = RootClass::Project;
};
struct Result {
  std::optional<Normalized> value;
  std::string error;
  bool ok() const { return value.has_value(); }
};
bool has_prefix(const std::filesystem::path &child,
                const std::filesystem::path &root) {
  auto c = child.begin(), r = root.begin();
  for (; r != root.end(); ++r, ++c) {
    if (c == child.end() || *c != *r)
      return false;
  }
  return true;
}
Result normalize(const std::filesystem::path &raw,
                 const std::filesystem::path &project_root,
                 const std::filesystem::path &build_root,
                 bool allow_external = false) {
  if (raw.empty())
    return {std::nullopt, "empty path"};
  auto p = project_root.lexically_normal();
  auto b = build_root.lexically_normal();
  if (!p.is_absolute() || !b.is_absolute())
    return {std::nullopt, "roots must be absolute"};
  std::filesystem::path abs =
      (raw.is_absolute() ? raw : (p / raw)).lexically_normal();
  if (has_prefix(abs, p))
    return {Normalized{abs, abs.lexically_relative(p), RootClass::Project}, {}};
  if (has_prefix(abs, b))
    return {Normalized{abs, abs.lexically_relative(b), RootClass::Build}, {}};
  if (allow_external && abs.is_absolute())
    return {Normalized{abs, abs, RootClass::External}, {}};
  return {std::nullopt, "path escapes declared roots"};
}
} // namespace d2t::path

namespace d2t::io {
struct ReadResult {
  std::optional<std::string> data;
  std::string error;
  bool ok() const { return data.has_value(); }
};
ReadResult read_text(const std::filesystem::path &p,
                     std::size_t max_bytes = 64U * 1024U * 1024U) {
  std::ifstream f(p, std::ios::binary);
  if (!f)
    return {std::nullopt, "cannot open file: " + p.string()};
  f.seekg(0, std::ios::end);
  auto n = f.tellg();
  if (n < 0)
    return {std::nullopt, "cannot size file: " + p.string()};
  if (static_cast<std::uint64_t>(n) > max_bytes)
    return {std::nullopt, "file too large: " + p.string()};
  f.seekg(0);
  std::string s(static_cast<std::size_t>(n), '\0');
  if (!s.empty())
    f.read(s.data(), static_cast<std::streamsize>(s.size()));
  if (!f && !s.empty())
    return {std::nullopt, "cannot read file: " + p.string()};
  return {std::move(s), {}};
}
} // namespace d2t::io

namespace d2t::ctest {
struct RegisteredTest {
  std::string name;
  std::vector<std::string> command;
};
struct Catalogue {
  std::vector<RegisteredTest> tests;
};
struct LoadResult {
  std::optional<Catalogue> catalogue;
  std::string error;
  bool ok() const { return catalogue.has_value(); }
};
LoadResult parse_catalogue(std::string_view text) {
  auto parsed = json::parse(text);
  if (!parsed.ok())
    return {std::nullopt, "invalid CTest JSON: " + parsed.error->code};
  const auto &root = *parsed.value;
  auto kind = json::find_member(root, "kind");
  auto version = json::find_member(root, "version");
  auto tests = json::find_member(root, "tests");
  if (!kind || !kind->is_string() || kind->as_string() != "ctestInfo")
    return {std::nullopt, "CTest kind must be ctestInfo"};
  if (!version || !version->is_object())
    return {std::nullopt, "CTest version missing"};
  auto major = json::find_member(*version, "major");
  if (!major || !major->is_number() || major->as_number() != 1.0)
    return {std::nullopt, "unsupported CTest major version"};
  if (!tests || !tests->is_array())
    return {std::nullopt, "CTest tests missing"};
  Catalogue out;
  std::set<std::string> names;
  for (const auto &tv : tests->as_array()) {
    auto name = json::find_member(tv, "name");
    auto command = json::find_member(tv, "command");
    if (!name || !name->is_string() || name->as_string().empty())
      return {std::nullopt, "invalid CTest test name"};
    if (!names.insert(name->as_string()).second)
      return {std::nullopt, "duplicate CTest test name"};
    if (!command || !command->is_array() || command->as_array().empty())
      return {std::nullopt, "invalid CTest command"};
    RegisteredTest t;
    t.name = name->as_string();
    for (const auto &x : command->as_array()) {
      if (!x.is_string())
        return {std::nullopt, "non-string CTest command token"};
      t.command.push_back(x.as_string());
    }
    out.tests.push_back(std::move(t));
  }
  std::sort(out.tests.begin(), out.tests.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });
  return {std::move(out), {}};
}
LoadResult load(const std::filesystem::path &p) {
  auto r = io::read_text(p);
  if (!r.ok())
    return {std::nullopt, r.error};
  return parse_catalogue(*r.data);
}
} // namespace d2t::ctest

namespace d2t::cmake {
struct Target {
  std::string id, name, type;
  std::vector<std::filesystem::path> sources, artifacts;
  std::vector<std::string> dependencies;
};
struct Model {
  std::filesystem::path source_root, build_root;
  std::string configuration;
  std::vector<Target> targets;
};
struct LoadResult {
  std::optional<Model> model;
  std::string error;
  bool ok() const { return model.has_value(); }
};
std::optional<std::filesystem::path>
safe_child(const std::filesystem::path &root, std::string_view rel) {
  std::filesystem::path p(rel);
  if (p.is_absolute())
    return std::nullopt;
  auto abs = (root / p).lexically_normal();
  if (!path::has_prefix(abs, root.lexically_normal()))
    return std::nullopt;
  return abs;
}
LoadResult load(const std::filesystem::path &reply,
                std::optional<std::filesystem::path> explicit_index,
                std::optional<std::string> config) {
  std::vector<std::filesystem::path> indexes;
  if (explicit_index) {
    indexes.push_back(*explicit_index);
  } else {
    std::error_code ec;
    for (const auto &e : std::filesystem::directory_iterator(reply, ec)) {
      if (ec)
        break;
      if (e.is_regular_file() &&
          e.path().filename().string().rfind("index-", 0) == 0 &&
          e.path().extension() == ".json")
        indexes.push_back(e.path());
    }
  }
  if (indexes.size() != 1)
    return {std::nullopt, "CMake reply index is missing or ambiguous"};
  auto ir = io::read_text(indexes[0]);
  if (!ir.ok())
    return {std::nullopt, ir.error};
  auto ip = json::parse(*ir.data);
  if (!ip.ok())
    return {std::nullopt, "invalid CMake index JSON"};
  auto rep = json::find_member(*ip.value, "reply");
  if (!rep || !rep->is_object())
    return {std::nullopt, "CMake reply missing"};
  auto cm = json::find_member(*rep, "codemodel-v2");
  if (!cm || !cm->is_object())
    return {std::nullopt, "codemodel-v2 missing"};
  auto ver = json::find_member(*cm, "version");
  auto jf = json::find_member(*cm, "jsonFile");
  if (!ver || !jf || !jf->is_string())
    return {std::nullopt, "codemodel reference invalid"};
  auto maj = json::find_member(*ver, "major");
  if (!maj || !maj->is_number() || maj->as_number() != 2.0)
    return {std::nullopt, "unsupported codemodel major"};
  auto cmfile = safe_child(reply, jf->as_string());
  if (!cmfile)
    return {std::nullopt, "codemodel reference escapes reply root"};
  auto cr = io::read_text(*cmfile);
  if (!cr.ok())
    return {std::nullopt, cr.error};
  auto cp = json::parse(*cr.data);
  if (!cp.ok())
    return {std::nullopt, "invalid codemodel JSON"};
  auto paths = json::find_member(*cp.value, "paths");
  auto configs = json::find_member(*cp.value, "configurations");
  if (!paths || !configs || !configs->is_array())
    return {std::nullopt, "codemodel structure invalid"};
  auto src = json::find_member(*paths, "source");
  auto build = json::find_member(*paths, "build");
  if (!src || !build || !src->is_string() || !build->is_string())
    return {std::nullopt, "codemodel paths invalid"};
  const json::Value *chosen = nullptr;
  for (const auto &c : configs->as_array()) {
    auto n = json::find_member(c, "name");
    if (!n || !n->is_string())
      continue;
    if (config) {
      if (n->as_string() == *config) {
        if (chosen)
          return {std::nullopt, "duplicate configuration"};
        chosen = &c;
      }
    } else {
      if (chosen)
        return {std::nullopt,
                "multiple configurations require --configuration"};
      chosen = &c;
    }
  }
  if (!chosen)
    return {std::nullopt, "configuration not found"};
  auto targets = json::find_member(*chosen, "targets");
  if (!targets || !targets->is_array())
    return {std::nullopt, "configuration targets missing"};
  Model out;
  out.source_root = src->as_string();
  out.build_root = build->as_string();
  out.configuration = config.value_or("");
  std::set<std::string> ids;
  for (const auto &tr : targets->as_array()) {
    auto id = json::find_member(tr, "id");
    auto file = json::find_member(tr, "jsonFile");
    if (!id || !file || !id->is_string() || !file->is_string())
      return {std::nullopt, "target reference invalid"};
    if (!ids.insert(id->as_string()).second)
      return {std::nullopt, "duplicate target id"};
    auto tf = safe_child(reply, file->as_string());
    if (!tf)
      return {std::nullopt, "target reference escapes reply root"};
    auto rr = io::read_text(*tf);
    if (!rr.ok())
      return {std::nullopt, rr.error};
    auto tp = json::parse(*rr.data);
    if (!tp.ok())
      return {std::nullopt, "invalid target JSON"};
    auto tid = json::find_member(*tp.value, "id");
    auto name = json::find_member(*tp.value, "name");
    auto type = json::find_member(*tp.value, "type");
    if (!tid || !name || !type || !tid->is_string() ||
        tid->as_string() != id->as_string() || !name->is_string() ||
        !type->is_string())
      return {std::nullopt, "target identity invalid"};
    Target t;
    t.id = id->as_string();
    t.name = name->as_string();
    t.type = type->as_string();
    if (auto s = json::find_member(*tp.value, "sources")) {
      if (!s->is_array())
        return {std::nullopt, "target sources invalid"};
      for (const auto &sv : s->as_array()) {
        auto p = json::find_member(sv, "path");
        if (!p || !p->is_string())
          return {std::nullopt, "source path invalid"};
        if (auto g = json::find_member(sv, "isGenerated");
            g && g->is_bool() && g->as_bool())
          return {std::nullopt, "generated source unsupported"};
        t.sources.emplace_back(p->as_string());
      }
    }
    if (auto a = json::find_member(*tp.value, "artifacts")) {
      if (!a->is_array())
        return {std::nullopt, "target artifacts invalid"};
      for (const auto &av : a->as_array()) {
        auto p = json::find_member(av, "path");
        if (!p || !p->is_string())
          return {std::nullopt, "artifact path invalid"};
        t.artifacts.emplace_back(p->as_string());
      }
    }
    if (auto d = json::find_member(*tp.value, "dependencies")) {
      if (!d->is_array())
        return {std::nullopt, "target dependencies invalid"};
      for (const auto &dv : d->as_array()) {
        auto p = json::find_member(dv, "id");
        if (!p || !p->is_string())
          return {std::nullopt, "dependency id invalid"};
        t.dependencies.push_back(p->as_string());
      }
    }
    out.targets.push_back(std::move(t));
  }
  std::sort(out.targets.begin(), out.targets.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });
  return {std::move(out), {}};
}
} // namespace d2t::cmake

namespace d2t::mapping {
struct TestMapping {
  std::string test_name, target_id;
};
struct Result {
  std::vector<TestMapping> mappings;
  std::vector<std::string> issues;
  bool complete() const { return issues.empty(); }
};
Result map_tests(const ctest::Catalogue &cat, const cmake::Model &model,
                 const std::filesystem::path &build_root) {
  std::map<std::filesystem::path, std::vector<std::string>> artifacts;
  for (const auto &t : model.targets)
    for (const auto &a : t.artifacts) {
      auto p = (a.is_absolute() ? a : (build_root / a)).lexically_normal();
      artifacts[p].push_back(t.id);
    }
  Result out;
  for (const auto &test : cat.tests) {
    std::filesystem::path cmd = test.command.front();
    auto p = (cmd.is_absolute() ? cmd : (build_root / cmd)).lexically_normal();
    auto it = artifacts.find(p);
    if (it == artifacts.end() || it->second.size() != 1) {
      out.issues.push_back("test " + test.name +
                           " does not map uniquely to a target artifact");
      continue;
    }
    out.mappings.push_back({test.name, it->second.front()});
  }
  std::sort(
      out.mappings.begin(), out.mappings.end(),
      [](const auto &a, const auto &b) { return a.test_name < b.test_name; });
  return out;
}
} // namespace d2t::mapping

namespace d2t::cli {
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
         "<build-root>/ctest-info.json\n  dependency list: "
         "<build-root>/deps.txt\n\n"
         "Overrides:\n  --project-root <dir>\n  --build <dir>\n  --build-root "
         "<dir>\n  --changed-files <file|->\n  --cmake-reply <dir>\n  "
         "--ctest-info <file>\n  --dep-list <file>\n  --cmake-index <file>\n  "
         "--configuration <name>\n  --format <human|names>\n  --explain\n  "
         "--verbose\n\nCMake, CTest, Git, and the compiler may generate input "
         "externally, but diff2test never launches them or any other program "
         "at "
         "runtime.\n";
}
ParseResult parse_analyze(int argc, char **argv) {
  ParseResult r;
  std::map<std::string, std::string> v;
  std::set<std::string> f;
  std::optional<std::string> positional_project_root;
  const std::set<std::string> vo = {
      "--project-root",  "--build",       "--build-root", "--changed-files",
      "--cmake-reply",   "--cmake-index", "--ctest-info", "--dep-list",
      "--configuration", "--format"};
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
    r.error =
        "project root specified both positionally and with --project-root";
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

  r.options.project_root =
      positional_project_root
          ? std::filesystem::path(*positional_project_root)
          : std::filesystem::path(
                v.contains("--project-root") ? v["--project-root"] : ".");
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
} // namespace d2t::cli
namespace d2t::analysis {
struct TranslationUnitKey {
  std::string target_id;
  std::filesystem::path source;
  bool operator<(const TranslationUnitKey &other) const {
    if (target_id != other.target_id)
      return target_id < other.target_id;
    return source.generic_string() < other.source.generic_string();
  }
};
struct DependencyEvidence {
  std::map<std::filesystem::path, std::set<TranslationUnitKey>>
      prerequisite_to_units;
  std::set<TranslationUnitKey> covered_units;
  std::map<TranslationUnitKey, std::filesystem::path> unit_to_file;
};
struct Result {
  core::Outcome outcome = core::Outcome::InternalError;
  std::vector<std::string> selected_tests;
  std::vector<std::string> reasons;
  std::map<std::string, std::vector<std::string>> explanations;
};
std::optional<std::vector<std::string>> read_lines(const std::string &source,
                                                   std::string &error) {
  std::vector<std::string> lines;
  std::istream *in = nullptr;
  std::ifstream file;
  if (source == "-")
    in = &std::cin;
  else {
    file.open(source);
    if (!file) {
      error = "cannot open line input: " + source;
      return std::nullopt;
    }
    in = &file;
  }
  std::string s;
  while (std::getline(*in, s)) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    if (s.find('\0') != std::string::npos) {
      error = "NUL in line input";
      return std::nullopt;
    }
    if (!s.empty())
      lines.push_back(s);
  }
  if (!*in && !in->eof()) {
    error = "failed while reading line input";
    return std::nullopt;
  }
  return lines;
}
std::optional<DependencyEvidence>
load_dependencies(const std::filesystem::path &list_file,
                  const cmake::Model &model,
                  const std::filesystem::path &project_root,
                  const std::filesystem::path &build_root, std::string &error) {
  std::map<std::filesystem::path, std::set<std::string>> source_owners;
  std::map<std::string, std::string> target_name_to_id;
  for (const auto &t : model.targets) {
    if (target_name_to_id.contains(t.name) &&
        target_name_to_id[t.name] != t.id) {
      error = "duplicate CMake target name is unsupported: " + t.name;
      return std::nullopt;
    }
    target_name_to_id[t.name] = t.id;
    for (const auto &source : t.sources) {
      auto sp = path::normalize(source, project_root, build_root, false);
      if (!sp.ok()) {
        error = "unsafe codemodel source: " + source.string();
        return std::nullopt;
      }
      if (sp.value->root_class == path::RootClass::Project &&
          sp.value->absolute.extension() == ".cpp")
        source_owners[sp.value->absolute].insert(t.id);
    }
  }
  auto lr = io::read_text(list_file, 4U * 1024U * 1024U);
  if (!lr.ok()) {
    error = lr.error;
    return std::nullopt;
  }
  std::istringstream in(*lr.data);
  std::string line;
  DependencyEvidence ev;
  std::set<std::filesystem::path> listed;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.empty())
      continue;
    std::filesystem::path dp = line;
    if (!dp.is_absolute())
      dp = (build_root / dp).lexically_normal();
    if (!listed.insert(dp).second) {
      error = "duplicate dependency file in --dep-list: " + dp.string();
      return std::nullopt;
    }
    auto dr = io::read_text(dp, 16U * 1024U * 1024U);
    if (!dr.ok()) {
      error = dr.error;
      return std::nullopt;
    }
    auto parsed = dep::parse(*dr.data);
    if (!parsed.ok()) {
      error =
          "invalid dependency file " + dp.string() + ": " + parsed.error->code;
      return std::nullopt;
    }
    if (parsed.rules.size() != 1) {
      error = "dependency file must contain exactly one compiler rule: " +
              dp.string();
      return std::nullopt;
    }
    std::error_code dep_time_error;
    const auto dep_time = std::filesystem::last_write_time(dp, dep_time_error);
    if (dep_time_error) {
      error = "cannot inspect dependency-file timestamp: " + dp.string();
      return std::nullopt;
    }
    const auto &rule = parsed.rules.front();
    std::set<std::string> rule_targets;
    for (const auto &token : rule.targets) {
      std::string g = std::filesystem::path(token).generic_string();
      for (const auto &[name, id] : target_name_to_id) {
        std::string marker = "CMakeFiles/" + name + ".dir/";
        if (g.find(marker) != std::string::npos)
          rule_targets.insert(id);
      }
    }
    if (rule_targets.size() != 1) {
      error =
          "dependency rule target does not map uniquely to a CMake target: " +
          dp.string();
      return std::nullopt;
    }
    const std::string target_id = *rule_targets.begin();
    std::set<std::filesystem::path> source_candidates;
    std::vector<std::filesystem::path> project_prereqs;
    for (const auto &prereq : rule.prerequisites) {
      auto np = path::normalize(prereq, project_root, build_root, true);
      if (!np.ok()) {
        error = "unsafe dependency prerequisite: " + prereq;
        return std::nullopt;
      }
      if (np.value->root_class == path::RootClass::Project) {
        project_prereqs.push_back(np.value->absolute);
        std::error_code prereq_time_error;
        const auto prereq_time = std::filesystem::last_write_time(
            np.value->absolute, prereq_time_error);
        if (prereq_time_error) {
          error = "cannot inspect dependency prerequisite timestamp: " +
                  np.value->absolute.string();
          return std::nullopt;
        }
        if (prereq_time > dep_time) {
          error = "stale dependency file " + dp.string() +
                  ": prerequisite is newer: " + np.value->absolute.string();
          return std::nullopt;
        }
        auto it = source_owners.find(np.value->absolute);
        if (it != source_owners.end() && it->second.contains(target_id))
          source_candidates.insert(np.value->absolute);
      }
    }
    if (source_candidates.size() != 1) {
      error = "dependency rule does not identify exactly one compiled source "
              "for its target: " +
              dp.string();
      return std::nullopt;
    }
    TranslationUnitKey unit{target_id, *source_candidates.begin()};
    if (!ev.covered_units.insert(unit).second) {
      error = "duplicate dependency evidence for translation unit: " +
              unit.source.string();
      return std::nullopt;
    }
    ev.unit_to_file.emplace(unit, dp);
    for (const auto &prereq : project_prereqs)
      ev.prerequisite_to_units[prereq].insert(unit);
  }
  for (const auto &[source, owners] : source_owners)
    for (const auto &target_id : owners) {
      TranslationUnitKey key{target_id, source};
      if (!ev.covered_units.contains(key)) {
        error = "missing dependency evidence for source " +
                source.lexically_relative(project_root).string() +
                " in target " + target_id;
        return std::nullopt;
      }
    }
  return ev;
}
Result analyze(const cli::AnalyzeOptions &opt) {
  Result out;
  std::error_code metadata_ec;
  if (!std::filesystem::exists(opt.ctest_info, metadata_ec) || metadata_ec) {
    out.outcome = core::Outcome::FullSuiteRequired;
    out.reasons.push_back("CTest catalogue not found at " +
                          opt.ctest_info.string());
    return out;
  }
  auto cat = ctest::load(opt.ctest_info);
  if (!cat.ok()) {
    out.outcome = core::Outcome::FullSuiteRequired;
    out.reasons.push_back(cat.error);
    return out;
  }
  auto all_names = [&]() {
    std::vector<std::string> v;
    for (const auto &t : cat.catalogue->tests)
      v.push_back(t.name);
    return v;
  };
  auto full = [&](std::string reason) {
    out.outcome = core::Outcome::FullSuiteSelected;
    out.selected_tests = all_names();
    out.reasons.push_back(std::move(reason));
    return out;
  };
  if (!std::filesystem::is_directory(opt.project_root) ||
      !std::filesystem::is_directory(opt.build_root))
    return full("project/build root must exist and be directories");
  if (!std::filesystem::is_directory(opt.cmake_reply))
    return full("CMake reply directory not found at " +
                opt.cmake_reply.string());
  if (!std::filesystem::is_directory(opt.cmake_reply))
    return full("CMake reply directory not found at " +
                opt.cmake_reply.string());
  auto model = cmake::load(opt.cmake_reply, opt.cmake_index, opt.configuration);
  if (!model.ok())
    return full(model.error);
  auto declared_project =
      std::filesystem::absolute(opt.project_root).lexically_normal();
  auto declared_build =
      std::filesystem::absolute(opt.build_root).lexically_normal();
  if (std::filesystem::absolute(model.model->source_root).lexically_normal() !=
          declared_project ||
      std::filesystem::absolute(model.model->build_root).lexically_normal() !=
          declared_build)
    return full("declared roots do not match codemodel paths");
  auto mapped =
      mapping::map_tests(*cat.catalogue, *model.model, declared_build);
  if (!mapped.complete()) {
    out.outcome = core::Outcome::FullSuiteSelected;
    out.selected_tests = all_names();
    out.reasons = mapped.issues;
    return out;
  }
  std::string err;
  if (!std::filesystem::exists(opt.dep_list))
    return full("dependency list not found at " + opt.dep_list.string());
  auto deps = load_dependencies(opt.dep_list, *model.model, declared_project,
                                declared_build, err);
  if (!deps)
    return full(err);
  auto changed = read_lines(opt.changed_files, err);
  if (!changed || changed->empty()) {
    out.outcome = core::Outcome::UsageError;
    out.reasons.push_back(changed ? "changed path list is empty" : err);
    return out;
  }
  std::sort(changed->begin(), changed->end());
  changed->erase(std::unique(changed->begin(), changed->end()), changed->end());
  std::set<TranslationUnitKey> affected_units;
  std::map<TranslationUnitKey, std::string> changed_for_unit;
  for (const auto &raw : *changed) {
    auto np = path::normalize(raw, declared_project, declared_build, false);
    if (!np.ok() || np.value->root_class != path::RootClass::Project)
      return full("unknown or unsafe changed path: " + raw);
    const auto abs = np.value->absolute;
    if (abs.filename() == "CMakeLists.txt" || abs.extension() == ".cmake")
      return full("build configuration changed: " + raw);
    auto it = deps->prerequisite_to_units.find(abs);
    if (it == deps->prerequisite_to_units.end())
      return full("unknown changed path: " + raw);
    for (const auto &unit : it->second) {
      affected_units.insert(unit);
      changed_for_unit.try_emplace(unit, raw);
    }
  }
  std::map<std::string, std::string> target_names;
  for (const auto &target : model.model->targets)
    target_names[target.id] = target.name;
  std::set<std::string> affected_targets;
  std::map<std::string, TranslationUnitKey> target_origin;
  for (const auto &unit : affected_units) {
    if (affected_targets.insert(unit.target_id).second)
      target_origin.emplace(unit.target_id, unit);
  }
  std::map<std::string, std::set<std::string>> reverse;
  std::set<std::string> ids;
  for (const auto &t : model.model->targets)
    ids.insert(t.id);
  for (const auto &t : model.model->targets)
    for (const auto &d : t.dependencies) {
      if (!ids.contains(d))
        return full("target dependency references unknown target: " + d);
      reverse[d].insert(t.id);
    }
  std::map<std::string, std::string> target_parent;
  std::queue<std::string> q;
  for (const auto &id : affected_targets)
    q.push(id);
  while (!q.empty()) {
    auto cur = q.front();
    q.pop();
    for (const auto &next : reverse[cur]) {
      if (affected_targets.insert(next).second) {
        target_parent[next] = cur;
        target_origin[next] = target_origin.at(cur);
        q.push(next);
      }
    }
  }
  std::map<std::string, std::string> test_to_target;
  for (const auto &m : mapped.mappings)
    test_to_target[m.test_name] = m.target_id;
  for (const auto &t : cat.catalogue->tests)
    if (affected_targets.contains(test_to_target[t.name]))
      out.selected_tests.push_back(t.name);
  std::sort(out.selected_tests.begin(), out.selected_tests.end());
  out.outcome = core::Outcome::SubsetSelected;
  for (const auto &name : out.selected_tests) {
    const auto target_id = test_to_target.at(name);
    const auto origin = target_origin.at(target_id);
    std::vector<std::string> steps;
    steps.push_back("changed path: " + changed_for_unit.at(origin));
    steps.push_back("dependency file: " +
                    deps->unit_to_file.at(origin)
                        .lexically_relative(declared_build)
                        .generic_string());
    steps.push_back(
        "translation unit: " +
        origin.source.lexically_relative(declared_project).generic_string());
    steps.push_back("owning target: " + target_names.at(origin.target_id));
    std::vector<std::string> chain;
    std::string current = target_id;
    while (current != origin.target_id) {
      chain.push_back(current);
      auto parent = target_parent.find(current);
      if (parent == target_parent.end())
        break;
      current = parent->second;
    }
    std::reverse(chain.begin(), chain.end());
    for (const auto &id : chain)
      steps.push_back("dependent target: " + target_names.at(id));
    steps.push_back("registered test: " + name);
    out.explanations[name] = std::move(steps);
  }
  return out;
}
} // namespace d2t::analysis
namespace d2t {
void emit(const analysis::Result &r, const cli::AnalyzeOptions &opt) {
  auto status = [&]() {
    switch (r.outcome) {
    case core::Outcome::SubsetSelected:
      return "SUBSET_SELECTED";
    case core::Outcome::FullSuiteSelected:
      return "FULL_SUITE_SELECTED";
    case core::Outcome::FullSuiteRequired:
      return "FULL_SUITE_REQUIRED";
    case core::Outcome::UsageError:
      return "USAGE_ERROR";
    case core::Outcome::InputError:
      return "INPUT_ERROR";
    case core::Outcome::InternalError:
      return "INTERNAL_ERROR";
    }
    return "INTERNAL_ERROR";
  };
  if (opt.format == "human") {
    std::cout << "STATUS: " << status() << "\n";
    if (!r.selected_tests.empty()) {
      std::cout << "\nSelected tests (" << r.selected_tests.size() << "):\n";
      for (const auto &n : r.selected_tests)
        std::cout << "  " << n << "\n";
    }
    if (opt.explain)
      for (const auto &[name, steps] : r.explanations) {
        std::cout << "\nReason for " << name << ":\n";
        for (const auto &s : steps)
          std::cout << "  " << s << "\n";
      }
  } else
    for (const auto &n : r.selected_tests)
      std::cout << n << "\n";
  for (const auto &reason : r.reasons)
    std::cerr << "diff2test: " << status() << ": " << reason << "\n";
}
int run(int argc, char **argv) {
  using core::Outcome;
  if (argc == 2) {
    std::string_view a = argv[1];
    if (a == "--help" || a == "-h") {
      cli::print_help(std::cout);
      return 0;
    }
    if (a == "--version") {
      std::cout << "diff2test " << core::kVersion << "\n";
      return 0;
    }
  }
  if (argc < 2 || std::string_view(argv[1]) != "analyze") {
    std::cerr << "diff2test: expected command 'analyze' (try --help)\n";
    return core::exit_code(Outcome::UsageError);
  }
  auto parsed = cli::parse_analyze(argc, argv);
  if (!parsed.ok) {
    std::cerr << "diff2test: " << parsed.error << "\n";
    return core::exit_code(Outcome::UsageError);
  }
  auto result = analysis::analyze(parsed.options);
  emit(result, parsed.options);
  return core::exit_code(result.outcome);
}
} // namespace d2t
#ifndef D2T_TESTING
int main(int argc, char **argv) {
  try {
    return d2t::run(argc, argv);
  } catch (const std::exception &ex) {
    std::cerr << "diff2test: internal error: " << ex.what() << "\n";
    return d2t::core::exit_code(d2t::core::Outcome::InternalError);
  } catch (...) {
    std::cerr << "diff2test: internal error: unknown exception\n";
    return d2t::core::exit_code(d2t::core::Outcome::InternalError);
  }
}
#endif
