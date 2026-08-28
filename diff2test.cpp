#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
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

namespace d2t::cli {

struct AnalyzeOptions {
    std::filesystem::path project_root;
    std::filesystem::path build_root;
    std::string changed_files;
    std::filesystem::path cmake_reply;
    std::optional<std::filesystem::path> cmake_index;
    std::filesystem::path ctest_info;
    std::optional<std::filesystem::path> dep_root;
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
        "  exactly one of --dep-root <dir> or --dep-list <file>\n\n"
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
        "--cmake-index", "--ctest-info", "--dep-root", "--dep-list",
        "--configuration", "--format"
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
        "--project-root", "--build-root", "--changed-files", "--cmake-reply", "--ctest-info"
    };
    for (const auto& option : required) {
        if (!values.contains(option)) {
            result.error = "missing required option: " + option;
            return result;
        }
    }

    const bool has_dep_root = values.contains("--dep-root");
    const bool has_dep_list = values.contains("--dep-list");
    if (has_dep_root == has_dep_list) {
        result.error = "exactly one of --dep-root or --dep-list is required";
        return result;
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
    if (values.contains("--cmake-index")) result.options.cmake_index = values["--cmake-index"];
    if (has_dep_root) result.options.dep_root = values["--dep-root"];
    if (has_dep_list) result.options.dep_list = values["--dep-list"];
    if (values.contains("--configuration")) result.options.configuration = values["--configuration"];
    if (values.contains("--format")) result.options.format = values["--format"];
    result.options.explain = flags.contains("--explain");
    result.options.verbose = flags.contains("--verbose");
    result.ok = true;
    return result;
}

}  // namespace d2t::cli

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

    // The metadata analyzers are intentionally not wired yet. Returning a usage-like
    // non-success status prevents this foundation commit from ever claiming a safe subset.
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
