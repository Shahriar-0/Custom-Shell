#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <string_view>
#include <vector>

#include "ast.hpp"
#include "syntax_error.hpp"
#include "token.hpp"

namespace parser {

// Parses a token stream into a CommandLine.
//
// Grammar (follows POSIX's list / and_or / pipeline layering; everything is
// left-associative, and a trailing separator is allowed):
//
//   command_line := list ( (';' | '&') list )* [';' | '&']
//   list         := pipeline ( ('&&' | '||') pipeline )*
//   pipeline     := command ('|' command)*
//   command      := ( WORD | redirection )+    -- at least one WORD(the program): (WORD|redirection)+WORD(WORD|redirection)+
//   redirection  := [IO_NUMBER] ('>' | '>>' | '<' | '>&' | '<&') WORD
//                |  ('&>' | '&>>') WORD
//
// A '&' terminating a list marks just that list as background; ';' only
// separates. `a && b || c` stays one list, which the executor walks left to
// right with short-circuiting.
//
// Throws SyntaxError on malformed input. On success, the returned
// CommandLine is well-formed: no empty commands, no dangling && / || / |,
// and every redirection has exactly one target word.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    CommandLine parse();

private:
    [[noreturn]] static void fail(const std::string& message, size_t column) {
        throw SyntaxError{message, column};
    }

    const Token& peek() const { return tokens_[pos_]; }

    const Token& advance() { return tokens_[pos_++]; }

    bool check(TokenType type) const { return peek().type == type; }

    bool atEnd() const { return check(TokenType::EndOfInput); }

    // True if the next token begins a redirection.
    bool atRedirectionStart() const;

    // Consumes the next token if it is `type`; returns nullptr otherwise.
    const Token* match(TokenType type);

    const Token& expect(TokenType type, const std::string& what);
    void expectWithoutAdvancing(TokenType type, const std::string& what) const;

    AndOrList parseAndOrList();
    Pipeline parsePipeline();
    Command parseCommand();
    // Appends one entry to `out`, or two for `&>` and `&>>`.
    void parseRedirection(std::vector<Redirection>& out);
    Connector parseConnector();

    std::vector<Token> tokens_;
    size_t pos_ = 0;
};

// Convenience wrapper: lex + parse in one call.
CommandLine parseLine(std::string_view input);

} // namespace parser

#endif // PARSER_HPP
