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

// Parses a token stream into a CommandLine.
//
// Grammar (left-associative at every level). This flattens ';', '&&' and
// '||' into one list-with-connectors rather than a nested tree; that's
// operationally equivalent to POSIX's nested and_or/list grammar for
// left-to-right execution with short-circuiting, and much simpler to walk:
//
//   command_line := pipeline ( (';' | '&&' | '||') pipeline )* ['&']
//   pipeline     := command ('|' command)*
//   command      := WORD+ redirection*
//   redirection  := ('>' | '>>' | '<') WORD
//
// Throws ParseError on malformed input. On success, the returned
// CommandLine is well-formed: no empty commands, no dangling operators,
// and every redirection has exactly one filename word.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    CommandLine parse();

private:
    [[noreturn]] static void fail(const std::string& message, size_t column) {
        throw ParseError{message, column};
    }

    const Token& peek() const { return tokens_[pos_]; }

    const Token& advance() { return tokens_[pos_++]; }

    bool check(TokenType type) const { return peek().type == type; }

    bool atEnd() const { return check(TokenType::EndOfInput); }

    // Consumes the next token if it is `type`; returns nullptr otherwise.
    const Token* match(TokenType type);

    const Token& expect(TokenType type, const std::string& what);

    void expectWordHere(const std::string& context);

    Pipeline parsePipeline();
    Command parseCommand();
    Redirection parseRedirection(RedirType type);
    Connector parseConnector();

    std::vector<Token> tokens_;
    size_t pos_ = 0;
};

// Convenience wrapper: lex + parse in one call.
CommandLine parseLine(std::string_view input);

} // namespace parser

#endif // PARSER_HPP
