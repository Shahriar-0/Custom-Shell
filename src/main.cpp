#include <iostream>
#include <string>

#include "shell_builtin_commands.hpp"

int main() {
    // Flush after every std::cout / std:cerr
    // so that the output is immediately visible
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

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
