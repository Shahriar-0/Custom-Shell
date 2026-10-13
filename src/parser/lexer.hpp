#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <string_view>
#include <vector>

#include "syntax_error.hpp"
#include "token.hpp"

namespace parser {

// Converts one line of shell input into typed tokens.
//
// Grammar handled here (lexical level):
//   - whitespace separates tokens
//   - '...' is fully literal; "..." is literal except for \" \\ \$ and \`
//   - backslash outside quotes escapes the next character
//   - # starts a comment (rest of line ignored) when unquoted
//   - an empty quote pair ("" or '') is a real, empty word
//   - operators: | || && & ; < > >> <& >& &> &>>  (only recognized outside quotes)
//   - an all-digit, fully unquoted word immediately followed by < or >
//     is an IoNumber token (the fd in `2>file`, `2>&1`); anywhere else it is a Word
//   - ( ) are reserved and rejected as syntax errors
//
// Throws SyntaxError on unterminated quotes, a trailing backslash, or
// stray parens. Where a single '&' is legal is the parser's call, not the
// lexer's.
std::vector<Token> lex(std::string_view input);

} // namespace parser

#endif // LEXER_HPP
