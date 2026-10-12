#include "lexer.hpp"

namespace parser {

namespace {

class Lexer {
public:
    explicit Lexer(std::string_view input) : input_(input) {}

    std::vector<Token> run() {
        std::vector<Token> tokens;
        while (true) {
            Token tok = next();
            if (tok.type == TokenType::EndOfInput) {
                break;
            }
            tokens.push_back(std::move(tok));
        }
        tokens.push_back(makeToken(TokenType::EndOfInput));
        return tokens;
    }

private:
    [[noreturn]] static void fail(const std::string& message, size_t column) {
        throw SyntaxError{message, column};
    }

    Token makeToken(TokenType type, std::string text = "", size_t column = 0,
                    std::string quoteKinds = "") const {
        return Token{type, std::move(text), column, std::move(quoteKinds)};
    }

    bool atEnd() const { return pos_ >= input_.size(); }

    char peek() const { return input_[pos_]; }

    void skipWhitespaceAndComments() {
        while (!atEnd()) {
            if (input_[pos_] == ' ' || input_[pos_] == '\t') {
                ++pos_;
                continue;
            }
            // Unquoted # begins a comment running to end of line.
            if (input_[pos_] == '#') {
                pos_ = input_.size();
                return;
            }
            return;
        }
    }

    Token next() {
        skipWhitespaceAndComments();
        startColumn_ = pos_ + 1;
        if (atEnd()) {
            return makeToken(TokenType::EndOfInput);
        }

        switch (char c = peek()) {
        case '|': return lexPipe();
        case '&': return lexAmpersand();
        case ';':
            advance();
            return makeToken(TokenType::Semicolon, ";");
        case '<':
            advance();
            return makeToken(TokenType::RedirectIn, "<");
        case '>': return lexRedirectOut();
        case '(':
        case ')':
            fail(std::string("unexpected '") + c +
                     "' (subshells/grouping are not supported yet)",
                 startColumn_);
        default: return lexWord();
        }
    }

    void advance() { ++pos_; }

    Token lexPipe() {
        advance();
        if (!atEnd() && peek() == '|') {
            advance();
            return makeToken(TokenType::Or, "||");
        }
        return makeToken(TokenType::Pipe, "|");
    }

    // '&&' is And; a lone '&' is Ampersand. The parser decides where it's legal.
    Token lexAmpersand() {
        advance();
        if (!atEnd() && peek() == '&') {
            advance();
            return makeToken(TokenType::And, "&&");
        }
        return makeToken(TokenType::Ampersand, "&");
    }

    Token lexRedirectOut() {
        advance();
        if (!atEnd() && peek() == '>') {
            advance();
            return makeToken(TokenType::RedirectAppend, ">>");
        }
        return makeToken(TokenType::RedirectOut, ">");
    }

    // Scans one word: bare characters, quoted runs, and backslash escapes
    // may all be mixed freely ("ab"c'd\' becomes abc'd').
    Token lexWord() {
        Token tok = makeToken(TokenType::Word);
        tok.column = startColumn_;

        while (!atEnd()) {
            char c = peek();

            // Word ends at whitespace or an operator character.
            if (c == ' ' || c == '\t' || isOperatorStart(c)) {
                break;
            }

            if (c == '\'') {
                lexSingleQuote(tok);
            }
            else if (c == '"') {
                lexDoubleQuote(tok);
            }
            else if (c == '\\') {
                lexEscape(tok, /*inDoubleQuotes=*/false);
            }
            else {
                appendChar(tok, c, QuoteKind::None);
                advance();
            }
        }

        // The word can be empty only as "" or '', which is a valid empty argument.

        // `2>file`: digits that are the *entire* word, none of them quoted
        // or escaped, touching a redirection operator, name the fd.
        if (isIoNumber(tok) && !atEnd() && (peek() == '<' || peek() == '>')) {
            tok.type = TokenType::IoNumber;
        }
        return tok;
    }

    static bool isIoNumber(const Token& tok) {
        if (tok.text.empty()) {
            return false;
        }
        for (size_t i = 0; i < tok.text.size(); ++i) {
            if (tok.text[i] < '0' || tok.text[i] > '9' ||
                tok.quoteKinds[i] != static_cast<char>(QuoteKind::None)) {
                return false;
            }
        }
        return true;
    }

    // True if this character always starts an operator token outside quotes.
    static bool isOperatorStart(char c) {
        switch (c) {
        case '|':
        case '&':
        case ';':
        case '<':
        case '>':
        case '(':
        case ')':
            return true;
        default:
            return false;
        }
    }

    void lexSingleQuote(Token& tok) {
        size_t openColumn = pos_ + 1;
        advance(); // consume opening '
        while (true) {
            if (atEnd()) {
                fail("unterminated single quote (opening at column " +
                         std::to_string(openColumn) + ")",
                     openColumn);
            }
            char c = peek();
            if (c == '\'') {
                advance();
                return; // everything inside was literal
            }
            appendChar(tok, c, QuoteKind::Single);
            advance();
        }
    }

    void lexDoubleQuote(Token& tok) {
        size_t openColumn = pos_ + 1;
        advance(); // consume opening "
        while (true) {
            if (atEnd()) {
                fail("unterminated double quote (opening at column " +
                         std::to_string(openColumn) + ")",
                     openColumn);
            }
            char c = peek();
            if (c == '"') {
                advance();
                return;
            }
            if (c == '\\') {
                lexEscape(tok, /*inDoubleQuotes=*/true);
                continue;
            }
            appendChar(tok, c, QuoteKind::Double);
            advance();
        }
    }

    // Backslash: outside quotes escapes anything; inside double quotes it
    // escapes only ", \, $ and `, otherwise the backslash stays literal.
    void lexEscape(Token& tok, bool inDoubleQuotes) {
        size_t escapeColumn = pos_ + 1;
        advance(); // consume backslash
        if (atEnd()) {
            fail("trailing backslash with nothing to escape", escapeColumn);
        }
        char c = peek();
        if (inDoubleQuotes && c != '"' && c != '\\' && c != '$' && c != '`') {
            // Not special: keep the backslash and the character as literals.
            appendChar(tok, '\\', QuoteKind::Double);
            appendChar(tok, c, QuoteKind::Double);
            advance();
            return;
        }
        appendChar(tok, c, QuoteKind::Escape);
        advance();
    }

    static void appendChar(Token& tok, char c, QuoteKind kind) {
        tok.text += c;
        tok.quoteKinds += static_cast<char>(kind);
    }

    std::string_view input_;
    size_t pos_ = 0;
    size_t startColumn_ = 0;
};

} // namespace

std::vector<Token> lex(std::string_view input) {
    return Lexer(input).run();
}

} // namespace parser
