#include "parser.hpp"

#include "lexer.hpp"

namespace parser {

const Token* Parser::match(TokenType type) {
    if (check(type)) {
        return &advance();
    }
    return nullptr;
}

const Token& Parser::expect(TokenType type, const std::string& what) {
    if (!check(type)) {
        const Token& tok = peek();
        fail("expected " + what + " but found '" + tok.text + "'", tok.column);
    }
    return advance();
}

void Parser::expectWordHere(const std::string& context) {
    if (atEnd()) {
        fail("expected a word after " + context + ", got end of input",
             peek().column);
    }
    if (!check(TokenType::Word)) {
        const Token& tok = peek();
        fail("expected a word after " + context + ", found '" + tok.text + "'",
             tok.column);
    }
}

CommandLine Parser::parse() {
    CommandLine result;

    // Empty / whitespace-only / all-comment line.
    if (atEnd()) {
        return result;
    }

    // Reject leading operators: "| cmd", "&& cmd", "> file", "; cmd"...
    switch (peek().type) {
    case TokenType::Pipe:
    case TokenType::And:
    case TokenType::Or:
    case TokenType::Semicolon:
    case TokenType::RedirectIn:
    case TokenType::RedirectOut:
    case TokenType::RedirectAppend:
    case TokenType::Ampersand:
        fail("unexpected '" + peek().text + "' at start of command",
             peek().column);
    default:
        break;
    }

    while (true) {
        PipelineLink link;
        link.pipeline = parsePipeline();

        Connector connector = parseConnector();
        link.connectorToNext = connector;
        result.pipelines.push_back(std::move(link));

        if (connector == Connector::None) {
            break;
        }
    }

    // Trailing '&' — background the whole line.
    if (match(TokenType::Ampersand)) {
        result.background = true;
    }

    expect(TokenType::EndOfInput, "end of input");
    return result;
}

Connector Parser::parseConnector() {
    if (check(TokenType::Semicolon)) {
        advance();
        return Connector::Sequence;
    }
    if (check(TokenType::And)) {
        advance();
        return Connector::And;
    }
    if (check(TokenType::Or)) {
        advance();
        return Connector::Or;
    }
    return Connector::None;
}

Pipeline Parser::parsePipeline() {
    Pipeline pipeline;

    pipeline.commands.push_back(parseCommand());
    while (match(TokenType::Pipe)) {
        // Reject "cmd | | cmd2" and trailing "cmd1 |" with a clear message;
        // a bare "cmd1 |" at end of input also falls through to
        // parseCommand()'s own "expected a command" error either way.
        if (!atEnd() && !check(TokenType::Word) && !check(TokenType::RedirectIn) &&
            !check(TokenType::RedirectOut) && !check(TokenType::RedirectAppend)) {
            fail("expected a command after '|', found '" + peek().text + "'",
                 peek().column);
        }
        pipeline.commands.push_back(parseCommand());
    }

    return pipeline;
}

Command Parser::parseCommand() {
    Command cmd;

    const Token& first = expect(TokenType::Word, "a command");
    cmd.program = first.text;

    while (true) {
        if (check(TokenType::Word)) {
            cmd.args.push_back(advance().text);
        }
        else if (check(TokenType::RedirectOut)) {
            cmd.redirections.push_back(parseRedirection(RedirType::Out));
        }
        else if (check(TokenType::RedirectAppend)) {
            cmd.redirections.push_back(parseRedirection(RedirType::Append));
        }
        else if (check(TokenType::RedirectIn)) {
            cmd.redirections.push_back(parseRedirection(RedirType::In));
        }
        else {
            break;
        }
    }

    return cmd;
}

Redirection Parser::parseRedirection(RedirType type) {
    advance(); // consume operator
    expectWordHere(type == RedirType::Append ? "'>>'"
                   : type == RedirType::Out  ? "'>'"
                                             : "'<'");
    Token target = advance();
    return Redirection{type, target.text};
}

CommandLine parseLine(std::string_view input) {
    return Parser(lex(input)).parse();
}

} // namespace parser
