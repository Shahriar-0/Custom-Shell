#include "parser.hpp"

#include <charconv>
#include <system_error>

#include "lexer.hpp"

namespace parser {

namespace {

// How a token reads in an error message.
std::string describe(const Token& tok) {
    if (tok.type == TokenType::EndOfInput) {
        return "end of input";
    }
    return "'" + tok.text + "'";
}

Word toWord(const Token& tok) {
    return Word{tok.text, tok.quoteKinds};
}

} // namespace

const Token* Parser::match(TokenType type) {
    if (check(type)) {
        return &advance();
    }
    return nullptr;
}

const Token& Parser::expect(TokenType type, const std::string& what) {
    if (!check(type)) {
        const Token& tok = peek();
        fail("expected " + what + " but found " + describe(tok), tok.column);
    }
    return advance();
}

bool Parser::atRedirectionStart() const {
    switch (peek().type) {
    case TokenType::IoNumber:
    case TokenType::RedirectIn:
    case TokenType::RedirectOut:
    case TokenType::RedirectAppend:
        return true;
    default:
        return false;
    }
}

void Parser::expectWordHere(const std::string& context) {
    if (!check(TokenType::Word)) {
        const Token& tok = peek();
        fail("expected a word after " + context + ", found " + describe(tok),
             tok.column);
    }
}

CommandLine Parser::parse() {
    CommandLine result;

    // Empty / whitespace-only / all-comment line.
    if (atEnd()) {
        return result;
    }

    // Reject a leading separator or connector: "| cmd", "&& cmd", "; cmd"...
    // (A leading redirection is fine: `> out cmd`, `2>err cmd`.)
    switch (peek().type) {
    case TokenType::Pipe:
    case TokenType::And:
    case TokenType::Or:
    case TokenType::Semicolon:
    case TokenType::Ampersand:
        fail("unexpected '" + peek().text + "' at start of command",
             peek().column);
    default:
        break;
    }

    while (true) {
        result.lists.push_back(parseAndOrList());

        // A list ends at ';', '&' (consumed here) or end of input.
        if (match(TokenType::Semicolon)) {
            // plain separator
        }
        else if (match(TokenType::Ampersand)) {
            result.lists.back().background = true;
        }
        else {
            break;
        }

        // A trailing ';' or '&' is fine. A second separator in a row fails in parseCommand.
        if (atEnd()) {
            break;
        }
    }

    expect(TokenType::EndOfInput, "end of input");
    return result;
}

AndOrList Parser::parseAndOrList() {
    AndOrList list;

    while (true) {
        PipelineLink link;
        link.pipeline = parsePipeline();
        link.connectorToNext = parseConnector();
        Connector connector = link.connectorToNext;
        list.pipelines.push_back(std::move(link));

        if (connector == Connector::None) {
            break;
        }
    }

    return list;
}

Connector Parser::parseConnector() {
    if (match(TokenType::And)) {
        return Connector::And;
    }
    if (match(TokenType::Or)) {
        return Connector::Or;
    }
    return Connector::None;
}

Pipeline Parser::parsePipeline() {
    Pipeline pipeline;

    pipeline.commands.push_back(parseCommand());
    while (match(TokenType::Pipe)) {
         // Gives "cmd | | cmd2" a clearer message; "cmd |" falls through to parseCommand.
        if (!atEnd() && !check(TokenType::Word) && !atRedirectionStart()) {
            fail("expected a command after '|', found " + describe(peek()),
                 peek().column);
        }
        pipeline.commands.push_back(parseCommand());
    }

    return pipeline;
}

// command := ( WORD | redirection )+, with at least one WORD. The first WORD
// is the program wherever it sits, so `> out echo hi` works; later words are
// arguments, interleaved freely with redirections.
Command Parser::parseCommand() {
    Command cmd;
    bool haveProgram = false;

    while (true) {
        if (check(TokenType::Word)) {
            Word word = toWord(advance());
            if (!haveProgram) {
                cmd.program = std::move(word);
                haveProgram = true;
            }
            else {
                cmd.args.push_back(std::move(word));
            }
        }
        else if (atRedirectionStart()) {
            cmd.redirections.push_back(parseRedirection());
        }
        else {
            break;
        }
    }

    if (!haveProgram) {
        const Token& tok = peek();
        fail("expected a command but found " + describe(tok), tok.column);
    }
    return cmd;
}

// redirection := [IO_NUMBER] ('>' | '>>' | '<') WORD
Redirection Parser::parseRedirection() {
    Redirection redir;
    bool explicitFd = false;

    if (check(TokenType::IoNumber)) {
        const Token& num = advance();
        auto [ptr, ec] = std::from_chars(num.text.data(),
                                         num.text.data() + num.text.size(), redir.fd);
        if (ec != std::errc{} || ptr != num.text.data() + num.text.size() || redir.fd < 0) {
            fail("file descriptor " + num.text + " is out of range", num.column);
        }
        explicitFd = true;
    }

    // The lexer guarantees an operator follows an IoNumber; check anyway.
    const Token& op = peek();
    switch (op.type) {
    case TokenType::RedirectOut:
        redir.type = RedirType::Out;
        break;
    case TokenType::RedirectAppend:
        redir.type = RedirType::Append;
        break;
    case TokenType::RedirectIn:
        redir.type = RedirType::In;
        break;
    default:
        fail("expected a redirection operator but found " + describe(op), op.column);
    }
    advance(); // consume operator

    if (!explicitFd) {
        redir.fd = (redir.type == RedirType::In) ? 0 : 1;
    }

    expectWordHere(redir.type == RedirType::Append ? "'>>'"
                   : redir.type == RedirType::Out  ? "'>'"
                                                   : "'<'");
    redir.target = toWord(advance());
    return redir;
}

CommandLine parseLine(std::string_view input) {
    return Parser(lex(input)).parse();
}

} // namespace parser
