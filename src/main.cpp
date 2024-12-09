#include <iostream>
#include <string>

int main() {
    // Flush after every std::cout / std:cerr
    // so that the output is immediately visible
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    while (true) {
        std::cout << "$ ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "exit 0") {
            return 0;
        }
        
        std::cout << input << ": not found" << std::endl;
    }
}
