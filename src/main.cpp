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

    while (true) {
        std::cout << "$ ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\n";
            break; // EOF (Ctrl+D)
        }

        // Lex + parse the whole line before executing anything: a line with
        // a syntax error never partially runs (Crafting Interpreters rule).
        parser::Pipeline pipeline;
        try {
            pipeline = parser::parseLine(input);
        } catch (const parser::LexError& e) {
            std::cerr << "myshell: syntax error: " << e.message
                      << " (column " << e.column << ")\n";
            variables::lastExitStatus = 2;
            continue;
        } catch (const parser::ParseError& e) {
            std::cerr << "myshell: syntax error: " << e.message
                      << " (column " << e.column << ")\n";
            variables::lastExitStatus = 2;
            continue;
        }

        if (pipeline.nodes.empty()) {
            continue;
        }

        executor::execute(pipeline);
    }

    return variables::lastExitStatus;
}
