#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "../diagnostics/source_location.h"
#include "../utils/memory_pool.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <istream>
#include <memory>

namespace compiler {

class Lexer {
public:
    explicit Lexer(std::istream& input, const std::string& filename = "<input>");
    
    // Token stream interface
    Token nextToken();
    Token peekToken(size_t lookahead = 0);
    void consumeToken();
    
    // Batch tokenization
    std::vector<Token> tokenize();
    
    // State
    bool isAtEnd() const;
    SourceLocation getCurrentLocation() const;
    
    // Error recovery
    void skipUntil(TokenType type);
    void synchronize();

private:
    std::istream& input_;
    std::string filename_;
    std::string source_;
    size_t currentPos_;
    uint32_t currentLine_;
    uint32_t currentColumn_;
    
    // Lookahead buffer
    std::vector<Token> tokenBuffer_;
    size_t bufferPos_;
    
    // Keyword map
    static std::unordered_map<std::string, TokenType> keywords_;
    static void initKeywords();
    
    // Character handling
    char current() const;
    char peek(size_t offset = 0) const;
    char advance();
    bool match(char expected);
    bool isAtEnd(size_t pos) const;
    
    // Tokenizers
    Token scanToken();
    Token scanIdentifier();
    Token scanNumber();
    Token scanString();
    Token scanOperator();
    Token scanComment();
    
    // Helpers
    void skipWhitespace();
    void skipNewlines();
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    SourceLocation makeLocation() const;
    Token makeToken(TokenType type, const std::string& lexeme);
    Token errorToken(const std::string& message);
};

} // namespace compiler

#endif // LEXER_H
