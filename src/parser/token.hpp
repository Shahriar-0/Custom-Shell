#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstddef>
#include <string>

namespace parser {

enum class TokenType {
    Word,           // a bare word or quoted string
    Pipe,           // |
    RedirectOut,    // >
    RedirectAppend, // >>
    RedirectIn,     // <
    And,            // &&
    Or,             // ||
    Semicolon,      // ;
    Ampersand,      // & (background marker; only legal at end of input)
    EndOfInput,     // sentinel: no more tokens
};

// Which quoting context each character of a Word token came from.
// Phase 4 (variable expansion) needs this to decide whether $VAR inside
// a word should expand — POSIX expands in double quotes but not single.
enum class QuoteKind : char {
    None = 'u',
    Single = 's',
    Double = 'd',
    Escape = 'e', // character was produced by a backslash escape
};

struct Token {
    TokenType type = TokenType::EndOfInput;
    std::string text;       // raw lexeme (quotes stripped for Word)
    size_t column = 0;      // 1-based column of first char in source
    std::string quoteKinds; // per-char QuoteKind, only meaningful for Word

    bool isWord() const { return type == TokenType::Word; }
};

} // namespace parser

#endif // TOKEN_HPP
