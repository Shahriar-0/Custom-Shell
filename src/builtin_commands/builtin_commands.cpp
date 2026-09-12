#include "builtin_commands.hpp"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>

#include "executables/executables.hpp"
#include "utils/utils.hpp"
#include "variables/variables.hpp"

namespace shell_builtin_commands {

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

int echo(const parser::Command& cmd) {
    std::cout << utils::join(cmd.args, " ") << "\n";
    return 0;
}

int shellExit(const parser::Command& cmd) {
    int status = 0;
    if (!cmd.args.empty()) {
        try {
            status = std::stoi(cmd.args[0]);
        }
        catch (const std::exception&) {
            std::cerr << std::format("exit: {}: numeric argument required\n", cmd.args[0]);
            std::exit(2);
        }
    }
    std::exit(status);
}

int help(const parser::Command&) {
    std::cout << "Available commands:\n";
    for (const auto& [name, fn] : shell_builtin_cmds) {
        std::cout << "  " << name << "\n";
    }
    return 0;
}

int clear(const parser::Command&) {
    // ANSI clear-screen + move cursor home.
    std::cout << "\033[2J\033[H";
    return 0;
}

int type(const parser::Command& cmd) {
    // Resolution order mirrors execution: builtins shadow PATH executables.
    if (cmd.args.empty()) {
        std::cout << "Usage: type [command]\n";
        return 1;
    }

    const std::string& command = cmd.args[0];
    if (shellBuiltinCommandExists(command)) {
        std::cout << std::format("{} is a shell builtin\n", command);
        return 0;
    }
    if (auto path = executables::getExecutablePath(command); path.has_value()) {
        std::cout << std::format("{} is {}\n", command, *path);
        return 0;
    }
    std::cerr << std::format("{}: not found\n", command);
    return 1;
}

int pwd(const parser::Command&) {
    std::cout << std::filesystem::current_path().string() << "\n";
    return 0;
}

int cd(const parser::Command& cmd) {
    // No argument means home, same as bash. A leading '~' is expanded before
    // any path checks; everything else goes through the normal path join.
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
