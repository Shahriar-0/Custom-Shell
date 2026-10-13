#include "builtin_commands.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>

#include "executables/executables.hpp"
#include "utils/utils.hpp"
#include "variables/variables.hpp"

namespace shell_builtin_commands {

namespace {

// Reports a failed write to stdout (`echo hi >&-`) the way bash does and
// clears the stream so the next prompt still prints. Returns `status`
// if the write went through.
int finishOutput(const char* name, int status) {
    if (std::cout.good()) {
        return status;
    }
    int err = errno;
    std::cout.clear();
    std::clearerr(stdout);
    std::cerr << name << ": write error: " << (err != 0 ? std::strerror(err) : "Input/output error") << "\n";
    return 1;
}

} // namespace

const std::unordered_map<std::string, CommandFunction> shell_builtin_cmds = {
    {std::string(EXIT), &shellExit},
    {std::string(ECHO), &echo},
    {std::string(HELP), &help},
    {std::string(CLEAR), &clear},
    {std::string(TYPE), &type},
    {std::string(PWD), &pwd},
    {std::string(CD), &cd},
};

bool shellBuiltinCommandExists(const std::string& command) {
    return shell_builtin_cmds.contains(command);
}

int echo(const parser::ExpandedCommand& cmd) {
    errno = 0;
    std::cout << utils::join(cmd.args, " ") << "\n";
    return finishOutput("echo", 0);
}

int shellExit(const parser::ExpandedCommand& cmd) {
    // Bare `exit` leaves with the status of the last command (bash's $?),
    // not a hardcoded 0.
    int status = variables::lastExitStatus;
    if (!cmd.args.empty()) {
        // Like bash, the first argument must be an integer *in full* ("3abc"
        // is an error, not 3) and is checked before the argument count; a bad
        // one exits the shell with status 2.
        const std::string& arg = cmd.args[0];
        errno = 0;
        char* end = nullptr;
        long value = std::strtol(arg.c_str(), &end, 10);
        if (arg.empty() || end == arg.c_str() || *end != '\0' || errno == ERANGE) {
            std::cerr << std::format("exit: {}: numeric argument required\n", arg);
            std::exit(2);
        }
        if (cmd.args.size() > 1) {
            // Like bash: refuse to exit, report status 1, shell stays alive.
            std::cerr << "exit: too many arguments\n";
            return 1;
        }
        status = static_cast<int>(value & 0xFF); // exit statuses are 8 bits
    }
    std::exit(status);
}

int help(const parser::ExpandedCommand&) {
    errno = 0;
    std::cout << "Available commands:\n";
    for (const auto& [name, fn] : shell_builtin_cmds) {
        std::cout << "  " << name << "\n";
    }
    return finishOutput("help", 0);
}

int clear(const parser::ExpandedCommand&) {
    // ANSI clear-screen + move cursor home.
    errno = 0;
    std::cout << "\033[2J\033[H";
    return finishOutput("clear", 0);
}

int type(const parser::ExpandedCommand& cmd) {
    // Resolution order mirrors execution: builtins shadow PATH executables.
    // Like bash, every argument is resolved in turn; the status is nonzero
    // if any of them is not found.
    errno = 0;
    if (cmd.args.empty()) {
        std::cout << "Usage: type [command]\n";
        return finishOutput("type", 1);
    }

    int status = 0;
    for (const auto& command : cmd.args) {
        if (shellBuiltinCommandExists(command)) {
            std::cout << std::format("{} is a shell builtin\n", command);
        }
        else if (auto path = executables::getExecutablePath(command); path.has_value()) {
            std::cout << std::format("{} is {}\n", command, *path);
        }
        else {
            std::cerr << std::format("{}: not found\n", command);
            status = 1;
        }
        if (!std::cout.good()) {
            break;
        }
    }
    return finishOutput("type", status);
}

int pwd(const parser::ExpandedCommand&) {
    errno = 0;
    std::cout << std::filesystem::current_path().string() << "\n";
    return finishOutput("pwd", 0);
}

int cd(const parser::ExpandedCommand& cmd) {
    // No argument means home, same as bash. A leading '~' is expanded before
    // any path checks; everything else goes through the normal path join.
    if (cmd.args.size() > 1) {
        std::cerr << "cd: too many arguments\n";
        return 1;
    }
    std::string target = cmd.args.empty() ? "~" : cmd.args[0];

    target = utils::expandHome(target);
    if (utils::isHomePath(target)) {
        std::cerr << "cd: HOME not set\n";
        return 1;
    }

    std::error_code ec;
    std::filesystem::path newPath = utils::isAbsolutePath(target)
                                        ? std::filesystem::path(target)
                                        : std::filesystem::current_path() / target;
    std::filesystem::current_path(newPath, ec);
    if (ec) {
        std::cerr << std::format("cd: {}: No such file or directory\n", target);
        return 1;
    }
    return 0;
}

} // namespace shell_builtin_commands
