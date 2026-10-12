#ifndef SYNTAX_ERROR_HPP
#define SYNTAX_ERROR_HPP

#include <cstddef>
#include <string>

namespace parser {

// Error for one line of input, thrown by both the lexer and the parser.
struct SyntaxError {
    std::string message;
    size_t column = 0; // 1-based position the error was detected at
};

} // namespace parser

#endif // SYNTAX_ERROR_HPP
