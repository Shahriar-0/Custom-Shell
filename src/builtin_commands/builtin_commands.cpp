#include "builtin_commands.hpp"

#include <filesystem>
#include <iostream>

namespace shell_builtin_commands {

// Define the command map
std::unordered_map<std::string, CommandFunction> shell_builtin_cmds = {
    {EXIT, &shellExit}, {ECHO, &echo}, {HELP, &help}, {CLEAR, &clear}, {TYPE, &type}, {PWD, &pwd}, {CD, &cd}};

bool shellBuiltinCommandExists(const std::string& command) {
    return shell_builtin_cmds.find(command) != shell_builtin_cmds.end();
}

// Function implementations
int echo(const std::string& args) {
    std::vector<std::string> argsVec = utils::split(args, variables::COMMAND_DELIMITER);
    for (const auto& arg : argsVec) {
        std::cout << arg;
        if (arg != argsVec.back()) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    return 0;
}

int shellExit(const std::string& args) {
    int status = 0;
    if (!args.empty()) {
        status = std::stoi(args);
    }
    std::exit(status);
    return 0;
}

int help(const std::string& args) {
    std::cout << "Available commands:" << std::endl;
    for (const auto& command : shell_builtin_cmds) {
        std::cout << command.first << std::endl;
    }
    return 0;
}

int clear(const std::string& args) {
    std::system("clear");
    return 0;
}

int type(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: type [command]" << std::endl;
        return 1;
    }

    if (shellBuiltinCommandExists(args)) {
        std::cout << args << " is a shell builtin" << std::endl;
    }
    else if (auto path = executables::getExecutablePath(args); path.has_value()) {
        std::cout << args << " is " << utils::remove(path.value(), "\"") << std::endl;
    }
    else {
        std::cerr << args << ": not found" << std::endl;
    }
    return 0;
}

int pwd(const std::string& args) {
    std::cout << utils::remove(std::filesystem::current_path().string(), "\"") << std::endl;
    return 0;
}

int cd(const std::string& args) {
    if (args.empty() || utils::isHomePath(args)) {
        const char* home = std::getenv("HOME");
        if (home) {
            std::filesystem::current_path(home);
        }
        else {
            std::cerr << "HOME environment variable not set" << std::endl;
        }
    }
    else if (utils::isAbsolutePath(args)) {
        try {
            std::filesystem::current_path(args);
        }
        catch (const std::exception& e) {
            // std::cerr << e.what() << std::endl;
            std::cerr << "cd: " << args << ": No such file or directory" << std::endl;
        }
    }
    else if (utils::isRelativePath(args)) {
        try {
            std::filesystem::current_path(std::filesystem::current_path() / args);
        }
        catch (const std::exception& e) {
            // std::cerr << e.what() << std::endl;
            std::cerr << "cd: " << args << ": No such file or directory" << std::endl;
        }
    }
    else {
        // How the hell did you get here
        std::cerr << "Invalid path" << std::endl;
    }
    return 0;
}

} // namespace shell_builtin_commands
