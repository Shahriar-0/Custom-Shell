#ifndef PARSER_HPP
#define PARSER_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "ast.hpp"
#include "token.hpp"

namespace parser {

struct ParseError {
    std::string message;
    size_t column = 0;
};

// Parses a token stream into a full-line Pipeline.
//
// Grammar (left-associative at every level):
//   line      := and_or (('&' {EOF}) | (';' | '&&' | '||') and_or)*
//   and_or    := pipeline ('|' pipeline)*        -- note: '|' binds tighter,
//                                                   so this level is really
//   pipeline  := command ('|' command)*           -- the pipeline level; see below
//   command   := WORD | redirection ...
//   redirection := ('>' | '>>' | '<') WORD
//
// Throws ParseError on malformed input. On success the returned Pipeline is
// guaranteed well-formed: no empty commands, no dangling operators, every
// redirection has exactly one filename word.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    Pipeline parse();

private:
    [[noreturn]] static void fail(const std::string& message, size_t column) {
        throw ParseError{message, column};
    }

    const Token& peek() const { return tokens_[pos_]; }

    // Like peek(), but skips over nothing — used to check "what's after X".
    const Token& peekNext() const { return tokens_[pos_ + 1]; }

    const Token& advance() { return tokens_[pos_++]; }

    bool check(TokenType type) const { return peek().type == type; }

    bool atEnd() const { return check(TokenType::EndOfInput); }

    // Consumes the next token if it is `type`; returns nullptr otherwise.
    const Token* match(TokenType type);

    const Token& expect(TokenType type, const std::string& what);

    void expectWordHere(const std::string& context);

    PipelineNode parsePipeline();
    Command parseCommand();
    Redirection parseRedirection(RedirType type);
    Connector parseConnector();

    std::vector<Token> tokens_;
    size_t pos_ = 0;
};

// Convenience wrapper: lex + parse in one call.
Pipeline parseLine(std::string_view input);

} // namespace parser

#endif // PARSER_HPP
