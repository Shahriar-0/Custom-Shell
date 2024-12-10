#include <iostream>
#include <string>

#include "shell_builtin_commands.hpp"
#include "utils.hpp"
#include "variables.hpp"

int main() {
    // Flush after every std::cout / std:cerr
    // So that the output is immediately visible
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

#ifdef _WIN32
    const char delimiter = ';';
#else
    const char delimiter = ':';
#endif

    const char* env = std::getenv("PATH");
    if (env != nullptr) {
        variables::PATHs = utils::split(env, delimiter);
    }

    while (true) {
        std::cout << "$ ";

        std::string input;
        std::getline(std::cin, input);

        std::string command = input.substr(0, input.find(" "));
        std::string args = input.substr(input.find(" ") + 1);

        if (shell_builtin_commands::shell_builtin_cmds.find(command) !=
            shell_builtin_commands::shell_builtin_cmds.end()) {
            shell_builtin_commands::shell_builtin_cmds[command](args);
        } else if (command.empty()) {
            continue;
        } else {
            std::cout << input << ": not found" << std::endl;
        }
    }
}
