#include "lexer.h"
#include "../diagnostics/error_reporter.h"
#include <cctype>
#include <sstream>
#include <algorithm>

namespace compiler {

std::unordered_map<std::string, TokenType> Lexer::keywords_;

void Lexer::initKeywords() {
    if (!keywords_.empty()) return;
    
    keywords_["fn"] = TokenType::KW_FN;
    keywords_["let"] = TokenType::KW_LET;
    keywords_["const"] = TokenType::KW_CONST;
    keywords_["if"] = TokenType::KW_IF;
    keywords_["else"] = TokenType::KW_ELSE;
    keywords_["while"] = TokenType::KW_WHILE;
    keywords_["for"] = TokenType::KW_FOR;
    keywords_["return"] = TokenType::KW_RETURN;
    keywords_["break"] = TokenType::KW_BREAK;
    keywords_["continue"] = TokenType::KW_CONTINUE;
    keywords_["struct"] = TokenType::KW_STRUCT;
    keywords_["enum"] = TokenType::KW_ENUM;
    keywords_["type"] = TokenType::KW_TYPE;
    keywords_["import"] = TokenType::KW_IMPORT;
    keywords_["export"] = TokenType::KW_EXPORT;
    keywords_["true"] = TokenType::KW_TRUE;
    keywords_["false"] = TokenType::KW_FALSE;
    keywords_["null"] = TokenType::KW_NULL;
    keywords_["mut"] = TokenType::KW_MUT;
    keywords_["ref"] = TokenType::KW_REF;
    keywords_["deref"] = TokenType::KW_DEREF;
    keywords_["match"] = TokenType::KW_MATCH;
    keywords_["where"] = TokenType::KW_WHERE;
    keywords_["impl"] = TokenType::KW_IMPL;
    keywords_["trait"] = TokenType::KW_TRAIT;
    keywords_["async"] = TokenType::KW_ASYNC;
    keywords_["await"] = TokenType::KW_AWAIT;
}

Lexer::Lexer(std::istream& input, const std::string& filename)
    : input_(input), filename_(filename), currentPos_(0), 
      currentLine_(1), currentColumn_(1), bufferPos_(0) {
    
    initKeywords();
    
    // Read entire source
    std::stringstream buffer;
    buffer << input_.rdbuf();
    source_ = buffer.str();
}

char Lexer::current() const {
    if (isAtEnd(currentPos_)) return '\0';
    return source_[currentPos_];
}

char Lexer::peek(size_t offset) const {
    size_t pos = currentPos_ + offset;
    if (isAtEnd(pos)) return '\0';
    return source_[pos];
}

char Lexer::advance() {
    if (isAtEnd(currentPos_)) return '\0';
    char c = source_[currentPos_++];
    if (c == '\n') {
        currentLine_++;
        currentColumn_ = 1;
    } else {
        currentColumn_++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (current() == expected) {
        advance();
        return true;
    }
    return false;
}

bool Lexer::isAtEnd(size_t pos) const {
    return pos >= source_.size();
}

bool Lexer::isAtEnd() const {
    return isAtEnd(currentPos_);
}

SourceLocation Lexer::makeLocation() const {
    return SourceLocation(currentLine_, currentColumn_, currentPos_, &filename_);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
    return Token(type, lexeme, currentLine_, currentColumn_ - static_cast<uint32_t>(lexeme.length()), 
                 currentPos_ - lexeme.length());
}

Token Lexer::errorToken(const std::string& message) {
    REPORT_ERROR(makeLocation(), message);
    return makeToken(TokenType::UNKNOWN, "");
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipNewlines() {
    while (current() == '\n') advance();
}

Token Lexer::scanIdentifier() {
    uint32_t startCol = currentColumn_;
    size_t startPos = currentPos_;
    
    while (isAlphaNumeric(current())) advance();
    
    std::string text = source_.substr(startPos, currentPos_ - startPos);
    
    // Check for keyword
    auto it = keywords_.find(text);
    TokenType type = (it != keywords_.end()) ? it->second : TokenType::IDENTIFIER;
    
    Token tok = makeToken(type, text);
    tok.column = startCol;
    return tok;
}

Token Lexer::scanNumber() {
    uint32_t startCol = currentColumn_;
    size_t startPos = currentPos_;
    bool isFloat = false;
    
    while (isDigit(current())) advance();
    
    if (current() == '.' && isDigit(peek(1))) {
        isFloat = true;
        advance(); // consume '.'
        while (isDigit(current())) advance();
    }
    
    // Scientific notation
    if (current() == 'e' || current() == 'E') {
        isFloat = true;
        advance();
        if (current() == '+' || current() == '-') advance();
        while (isDigit(current())) advance();
    }
    
    // Type suffixes (i32, f64, etc.)
    if (current() == 'i' || current() == 'f' || current() == 'u') {
        advance();
        while (isDigit(current())) advance();
    }
    
    std::string text = source_.substr(startPos, currentPos_ - startPos);
    Token tok = makeToken(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL, text);
    tok.column = startCol;
    
    // Parse value
    try {
        if (isFloat) {
            tok.floatValue = std::stod(text);
        } else {
            tok.intValue = std::stoll(text);
        }
    } catch (...) {
        return errorToken("Invalid numeric literal: " + text);
    }
    
    return tok;
}

Token Lexer::scanString() {
    uint32_t startCol = currentColumn_;
    size_t startPos = currentPos_;
    
    advance(); // consume opening "
    std::string value;
    
    while (current() != '"' && !isAtEnd()) {
        if (current() == '\n') {
            return errorToken("Unterminated string literal");
        }
        
        if (current() == '\\') {
            advance();
            char c = current();
            switch (c) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '0': value += '\0'; break;
                default:
                    return errorToken("Invalid escape sequence: \\" + std::string(1, c));
            }
            advance();
        } else {
            value += advance();
        }
    }
    
    if (isAtEnd()) {
        return errorToken("Unterminated string literal");
    }
    
    advance(); // consume closing "
    
    Token tok = makeToken(TokenType::STRING_LITERAL, source_.substr(startPos, currentPos_ - startPos));
    tok.column = startCol;
    tok.stringValue = value;
    return tok;
}

Token Lexer::scanOperator() {
    uint32_t startCol = currentColumn_;
    char c = current();
    advance();
    
    switch (c) {
        case '+':
            if (match('+')) return makeToken(TokenType::INC, "++");
            if (match('=')) return makeToken(TokenType::PLUS_ASSIGN, "+=");
            return makeToken(TokenType::PLUS, "+");
            
        case '-':
            if (match('>')) return makeToken(TokenType::ARROW, "->");
            if (match('-')) return makeToken(TokenType::DEC, "--");
            if (match('=')) return makeToken(TokenType::MINUS_ASSIGN, "-=");
            return makeToken(TokenType::MINUS, "-");
            
        case '*':
            if (match('=')) return makeToken(TokenType::STAR_ASSIGN, "*=");
            return makeToken(TokenType::STAR, "*");
            
        case '/':
            if (match('/')) return scanComment();
            if (match('=')) return makeToken(TokenType::SLASH_ASSIGN, "/=");
            return makeToken(TokenType::SLASH, "/");
            
        case '%':
            if (match('=')) return makeToken(TokenType::PERCENT_ASSIGN, "%=");
            return makeToken(TokenType::PERCENT, "%");
            
        case '=':
            if (match('=')) return makeToken(TokenType::EQ, "==");
            if (match('>')) return makeToken(TokenType::FAT_ARROW, "=>");
            return makeToken(TokenType::ASSIGN, "=");
            
        case '!':
            if (match('=')) return makeToken(TokenType::NE, "!=");
            return makeToken(TokenType::NOT, "!");
            
        case '<':
            if (match('=')) return makeToken(TokenType::LE, "<=");
            if (match('<')) return makeToken(TokenType::SHL, "<<");
            return makeToken(TokenType::LT, "<");
            
        case '>':
            if (match('=')) return makeToken(TokenType::GE, ">=");
            if (match('>')) return makeToken(TokenType::SHR, ">>");
            return makeToken(TokenType::GT, ">");
            
        case '&':
            if (match('&')) return makeToken(TokenType::AND, "&&");
            return makeToken(TokenType::BIT_AND, "&");
            
        case '|':
            if (match('|')) return makeToken(TokenType::OR, "||");
            return makeToken(TokenType::BIT_OR, "|");
            
        case '^':
            return makeToken(TokenType::BIT_XOR, "^");
            
        case '~':
            return makeToken(TokenType::BIT_NOT, "~");
            
        case '(':
            return makeToken(TokenType::LPAREN, "(");
        case ')':
            return makeToken(TokenType::RPAREN, ")");
        case '{':
            return makeToken(TokenType::LBRACE, "{");
        case '}':
            return makeToken(TokenType::RBRACE, "}");
        case '[':
            return makeToken(TokenType::LBRACKET, "[");
        case ']':
            return makeToken(TokenType::RBRACKET, "]");
        case ';':
            return makeToken(TokenType::SEMICOLON, ";");
        case ':':
            if (match(':')) return makeToken(TokenType::DOUBLE_COLON, "::");
            return makeToken(TokenType::COLON, ":");
        case ',':
            return makeToken(TokenType::COMMA, ",");
        case '.':
            if (match('.') && match('.')) return makeToken(TokenType::ELLIPSIS, "...");
            return makeToken(TokenType::DOT, ".");
        case '?':
            return makeToken(TokenType::QUESTION, "?");
            
        default:
            return errorToken("Unexpected character: " + std::string(1, c));
    }
}

Token Lexer::scanComment() {
    // Line comment //
    while (current() != '\n' && !isAtEnd()) advance();
    return makeToken(TokenType::LINE_COMMENT, "");
}

Token Lexer::scanToken() {
    skipWhitespace();
    
    if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, "");
    
    char c = current();
    
    if (c == '\n') {
        advance();
        return makeToken(TokenType::NEWLINE, "\n");
    }
    
    if (isAlpha(c) || c == '_') return scanIdentifier();
    if (isDigit(c)) return scanNumber();
    if (c == '"') return scanString();
    
    return scanOperator();
}

Token Lexer::nextToken() {
    if (bufferPos_ < tokenBuffer_.size()) {
        return tokenBuffer_[bufferPos_++];
    }
    return scanToken();
}

Token Lexer::peekToken(size_t lookahead) {
    // Fill buffer if needed
    while (tokenBuffer_.size() <= bufferPos_ + lookahead) {
        tokenBuffer_.push_back(scanToken());
    }
    return tokenBuffer_[bufferPos_ + lookahead];
}

void Lexer::consumeToken() {
    if (bufferPos_ < tokenBuffer_.size()) {
        bufferPos_++;
    } else {
        scanToken(); // discard
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token tok;
    do {
        tok = nextToken();
        if (tok.type != TokenType::LINE_COMMENT && 
            tok.type != TokenType::BLOCK_COMMENT &&
            tok.type != TokenType::WHITESPACE) {
            tokens.push_back(tok);
        }
    } while (tok.type != TokenType::END_OF_FILE);
    return tokens;
}

void Lexer::skipUntil(TokenType type) {
    while (current() != '\0') {
        Token tok = nextToken();
        if (tok.type == type || tok.type == TokenType::END_OF_FILE) break;
    }
}

void Lexer::synchronize() {
    // Error recovery: skip to statement boundary
    while (current() != '\0') {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peekToken().type) {
            case TokenType::KW_FN:
            case TokenType::KW_LET:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
                return;
            default:
                consumeToken();
        }
    }
}

bool Lexer::isDigit(char c) const { return c >= '0' && c <= '9'; }
bool Lexer::isAlpha(char c) const { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool Lexer::isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c) || c == '_'; }

SourceLocation Lexer::getCurrentLocation() const { return makeLocation(); }

} // namespace compiler
