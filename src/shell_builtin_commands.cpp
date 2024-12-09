#include "shell_builtin_commands.hpp"
#include <iostream>

namespace shell_builtin_commands {

// Define the command map
std::unordered_map<std::string, CommandFunction> shell_builtin_cmds = {
    { EXIT, &shellExit },
    { ECHO, &echo },
    { HELP, &help },
    { CLEAR, &clear },
    { TYPE, &type },
};

// Function implementations
int echo(const std::string& args) {
    std::cout << args << std::endl;
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

    // If the command exists in shell_builtin_commands::shell_builtin_cmds
    if (shell_builtin_cmds.find(args) != shell_builtin_cmds.end()) {
        std::cout << args << " is a shell builtin" << std::endl;
    } else {
        // TODO: Check if the command exists in the PATH
        std::cout << args << ": not found" << std::endl;
    }

    return 0;
}

} // namespace commands
