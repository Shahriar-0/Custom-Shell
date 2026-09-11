#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <string_view>
#include <vector>

#include "token.hpp"

namespace parser {

struct LexError {
    std::string message;
    size_t column = 0; // 1-based position the error was detected at
};

// Converts one line of shell input into typed tokens.
//
// Grammar handled here (lexical level):
//   - whitespace separates tokens
//   - '...' is fully literal; "..." is literal except \" and \\
//   - backslash outside quotes escapes the next character
//   - # starts a comment (rest of line ignored) when unquoted
//   - operators: | || && & ; < > >>  (only recognized outside quotes)
//   - ( ) are reserved and rejected as syntax errors
//
// Throws LexError on unterminated quotes, invalid use of &, or stray parens.
std::vector<Token> lex(std::string_view input);

} // namespace parser

#endif // LEXER_HPP
