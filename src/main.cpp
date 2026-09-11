#include <iostream>
#include <string>

#include "executor/executor.hpp"
#include "parser/ast.hpp"
#include "parser/lexer.hpp"
#include "parser/parser.hpp"
#include "variables/variables.hpp"

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    variables::loadFromEnvironment();

    auto reportSyntaxError = [](const std::string& message, size_t column) {
        std::cerr << "myshell: syntax error: " << message
                  << " (column " << column << ")\n";
        variables::lastExitStatus = 2;
    };

    while (true) {
        std::cout << "$ ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\n";
            break; // EOF (Ctrl+D)
        }

        // Lex + parse the whole line before executing anything: a line with
        // a syntax error never partially runs.
        parser::CommandLine line;
        try {
            line = parser::parseLine(input);
        }
        catch (const parser::LexError& e) {
            reportSyntaxError(e.message, e.column);
            continue;
        }
        catch (const parser::ParseError& e) {
            reportSyntaxError(e.message, e.column);
            continue;
        }

        if (line.pipelines.empty()) {
            continue;
        }

        executor::execute(line);
    }

    return variables::lastExitStatus;
}
