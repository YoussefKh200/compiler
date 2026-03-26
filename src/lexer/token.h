#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <cstdint>

namespace compiler {

enum class TokenType {
    // End of file
    END_OF_FILE,
    
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,
    
    // Identifiers
    IDENTIFIER,
    
    // Keywords
    KW_FN,          // fn
    KW_LET,         // let
    KW_CONST,       // const
    KW_IF,          // if
    KW_ELSE,        // else
    KW_WHILE,       // while
    KW_FOR,         // for
    KW_RETURN,      // return
    KW_BREAK,       // break
    KW_CONTINUE,    // continue
    KW_STRUCT,      // struct
    KW_ENUM,        // enum
    KW_TYPE,        // type
    KW_IMPORT,      // import
    KW_EXPORT,      // export
    KW_TRUE,        // true
    KW_FALSE,       // false
    KW_NULL,        // null
    KW_MUT,         // mut (mutable)
    KW_REF,         // ref
    KW_DEREF,       // deref
    KW_MATCH,       // match
    KW_WHERE,       // where
    KW_IMPL,        // impl
    KW_TRAIT,       // trait
    KW_ASYNC,       // async
    KW_AWAIT,       // await
    
    // Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    ASSIGN,         // =
    PLUS_ASSIGN,    // +=
    MINUS_ASSIGN,   // -=
    STAR_ASSIGN,    // *=
    SLASH_ASSIGN,   // /=
    PERCENT_ASSIGN, // %=
    
    // Comparison
    EQ,             // ==
    NE,             // !=
    LT,             // <
    GT,             // >
    LE,             // <=
    GE,             // >=
    
    // Logical
    AND,            // &&
    OR,             // ||
    NOT,            // !
    
    // Bitwise
    BIT_AND,        // &
    BIT_OR,         // |
    BIT_XOR,        // ^
    BIT_NOT,        // ~
    SHL,            // <<
    SHR,            // >>
    
    // Increment/Decrement
    INC,            // ++
    DEC,            // --
    
    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMICOLON,      // ;
    COLON,          // :
    COMMA,          // ,
    DOT,            // .
    ARROW,          // ->
    FAT_ARROW,      // =>
    DOUBLE_COLON,   // ::
    ELLIPSIS,       // ...
    QUESTION,       // ?
    
    // Comments (skipped, but tracked for position)
    LINE_COMMENT,
    BLOCK_COMMENT,
    
    // Special
    NEWLINE,
    WHITESPACE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    uint32_t line;
    uint32_t column;
    uint32_t offset;
    
    // For literals
    union {
        int64_t intValue;
        double floatValue;
        bool boolValue;
    };
    std::string stringValue;

    Token() : type(TokenType::END_OF_FILE), line(1), column(1), offset(0), intValue(0) {}
    
    Token(TokenType t, std::string lex, uint32_t l, uint32_t c, uint32_t o)
        : type(t), lexeme(std::move(lex)), line(l), column(c), offset(o), intValue(0) {}

    bool isLiteral() const {
        return type == TokenType::INTEGER_LITERAL ||
               type == TokenType::FLOAT_LITERAL ||
               type == TokenType::STRING_LITERAL ||
               type == TokenType::BOOL_LITERAL;
    }

    bool isOperator() const {
        return type >= TokenType::PLUS && type <= TokenType::DEC;
    }

    bool isKeyword() const {
        return type >= TokenType::KW_FN && type <= TokenType::KW_AWAIT;
    }

    std::string toString() const;
};

} // namespace compiler

#endif // TOKEN_H
