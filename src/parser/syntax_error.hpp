#ifndef SYNTAX_ERROR_HPP
#define SYNTAX_ERROR_HPP

#include <cstddef>
#include <string>

namespace parser {

// The single error type for everything that can be wrong with one line of
// input, whether the lexer or the parser noticed it. main() reports both
// the same way, so there is no reason for callers to tell them apart.
struct SyntaxError {
    std::string message;
    size_t column = 0; // 1-based position the error was detected at
};

} // namespace parser

#endif // SYNTAX_ERROR_HPP
