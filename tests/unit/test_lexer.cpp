#include <gtest/gtest.h>
#include "lexer/lexer.h"
#include <sstream>

using namespace compiler;

TEST(LexerTest, BasicTokens) {
    std::istringstream input("fn main() { return 42; }");
    Lexer lexer(input, "test");
    
    auto tokens = lexer.tokenize();
    
    EXPECT_EQ(tokens[0].type, TokenType::KW_FN);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "main");
    EXPECT_EQ(tokens[2].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[3].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[4].type, TokenType::LBRACE);
    EXPECT_EQ(tokens[5].type, TokenType::KW_RETURN);
    EXPECT_EQ(tokens[6].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[6].intValue, 42);
    EXPECT_EQ(tokens[7].type, TokenType::SEMICOLON);
    EXPECT_EQ(tokens[8].type, TokenType::RBRACE);
}

TEST(LexerTest, Operators) {
    std::istringstream input("+ - * / == != <= >=");
    Lexer lexer(input, "test");
    
    auto tokens = lexer.tokenize();
    
    EXPECT_EQ(tokens[0].type, TokenType::PLUS);
    EXPECT_EQ(tokens[1].type, TokenType::MINUS);
    EXPECT_EQ(tokens[2].type, TokenType::STAR);
    EXPECT_EQ(tokens[3].type, TokenType::SLASH);
    EXPECT_EQ(tokens[4].type, TokenType::EQ);
    EXPECT_EQ(tokens[5].type, TokenType::NE);
    EXPECT_EQ(tokens[6].type, TokenType::LE);
    EXPECT_EQ(tokens[7].type, TokenType::GE);
}

TEST(LexerTest, Literals) {
    std::istringstream input("123 45.67 true false \"hello\"");
    Lexer lexer(input, "test");
    
    auto tokens = lexer.tokenize();
    
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[0].intValue, 123);
    EXPECT_EQ(tokens[1].type, TokenType::FLOAT_LITERAL);
    EXPECT_DOUBLE_EQ(tokens[1].floatValue, 45.67);
    EXPECT_EQ(tokens[2].type, TokenType::KW_TRUE);
    EXPECT_EQ(tokens[3].type, TokenType::KW_FALSE);
    EXPECT_EQ(tokens[4].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[4].stringValue, "hello");
}

TEST(LexerTest, Keywords) {
    std::istringstream input("fn let const if else while for return");
    Lexer lexer(input, "test");
    
    auto tokens = lexer.tokenize();
    
    EXPECT_EQ(tokens[0].type, TokenType::KW_FN);
    EXPECT_EQ(tokens[1].type, TokenType::KW_LET);
    EXPECT_EQ(tokens[2].type, TokenType::KW_CONST);
    EXPECT_EQ(tokens[3].type, TokenType::KW_IF);
    EXPECT_EQ(tokens[4].type, TokenType::KW_ELSE);
    EXPECT_EQ(tokens[5].type, TokenType::KW_WHILE);
    EXPECT_EQ(tokens[6].type, TokenType::KW_FOR);
    EXPECT_EQ(tokens[7].type, TokenType::KW_RETURN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
