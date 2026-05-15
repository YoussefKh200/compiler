#include <gtest/gtest.h>
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "utils/memory_pool.h"
#include <sstream>

using namespace compiler;

class ParserTest : public ::testing::Test {
protected:
    std::unique_ptr<Program> parse(const std::string& code) {
        std::istringstream input(code);
        Lexer lexer(input, "test");
        MemoryPool pool;
        Parser parser(lexer, pool);
        return parser.parseProgram();
    }
};

TEST_F(ParserTest, EmptyProgram) {
    auto ast = parse("");
    EXPECT_TRUE(ast->declarations.empty());
}

TEST_F(ParserTest, SimpleFunction) {
    auto ast = parse("fn main() { }");
    ASSERT_EQ(ast->declarations.size(), 1);
    
    auto* func = dynamic_cast<FunctionDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "main");
    EXPECT_TRUE(func->parameters.empty());
}

TEST_F(ParserTest, FunctionWithParams) {
    auto ast = parse("fn add(x: i32, y: i32) -> i32 { return x + y; }");
    ASSERT_EQ(ast->declarations.size(), 1);
    
    auto* func = dynamic_cast<FunctionDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "add");
    ASSERT_EQ(func->parameters.size(), 2);
    EXPECT_EQ(func->parameters[0].name, "x");
    EXPECT_EQ(func->parameters[1].name, "y");
}

TEST_F(ParserTest, VariableDeclaration) {
    auto ast = parse("fn main() { let x: i32 = 42; }");
    auto* func = dynamic_cast<FunctionDecl*>(ast->declarations[0].get());
    auto* block = dynamic_cast<BlockStmt*>(func->body.get());
    ASSERT_EQ(block->statements.size(), 1);
    
    auto* varDecl = dynamic_cast<VarDeclStmt*>(block->statements[0].get());
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->name, "x");
    EXPECT_FALSE(varDecl->isMutable);
}

TEST_F(ParserTest, IfStatement) {
    auto ast = parse("fn main() { if (true) { } }");
    auto* func = dynamic_cast<FunctionDecl*>(ast->declarations[0].get());
    auto* block = dynamic_cast<BlockStmt*>(func->body.get());
    auto* ifStmt = dynamic_cast<IfStmt*>(block->statements[0].get());
    
    ASSERT_NE(ifStmt, nullptr);
    ASSERT_NE(ifStmt->condition, nullptr);
    ASSERT_NE(ifStmt->thenBranch, nullptr);
    EXPECT_EQ(ifStmt->elseBranch, nullptr);
}

TEST_F(ParserTest,
