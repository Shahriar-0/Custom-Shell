#include "shell_builtin_commands.hpp"

#include <iostream>
#include <filesystem>



namespace shell_builtin_commands {

// Define the command map
std::unordered_map<std::string, CommandFunction> shell_builtin_cmds = {
    {EXIT, &shellExit}, {ECHO, &echo}, {HELP, &help}, {CLEAR, &clear}, {TYPE, &type},
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

    if (shell_builtin_cmds.find(args) != shell_builtin_cmds.end()) {
        // If the command exists in shell_builtin_commands::shell_builtin_cmds
        std::cout << args << " is a shell builtin" << std::endl;
    }
    else if (variables::PATHs.size() > 0) {
        // If the command exists in PATH
        for (const auto& path : variables::PATHs) {
            std::filesystem::path command_path = path + "/" + args;

            // Check if the file exists and is executable
            if (std::filesystem::exists(command_path) &&
                ((std::filesystem::status(command_path).permissions() & std::filesystem::perms::owner_exec) !=
                  std::filesystem::perms::none)) {


                std::cout << args << " is " << utils::remove(command_path.string(), path) << std::endl;
                return 0;
            }
        }
    }
    else {
        // If the command does not exist
        std::cout << args << ": not found" << std::endl;
    }
    return 0;
}

}  // namespace shell_builtin_commands
