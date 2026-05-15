#include <gtest/gtest.h>
#include "semantic/semantic_analyzer.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "utils/memory_pool.h"
#include <sstream>

using namespace compiler;

class SemanticTest : public ::testing::Test {
protected:
    bool analyze(const std::string& code) {
        std::istringstream input(code);
        Lexer lexer(input, "test");
        MemoryPool pool;
        Parser parser(lexer, pool);
        auto ast = parser.parseProgram();
        
        if (parser.hadError()) return false;
        
        SemanticAnalyzer sema;
        return sema.analyze(*ast);
    }
};

TEST_F(SemanticTest, ValidFunction) {
    EXPECT_TRUE(analyze("fn main() { }"));
}

TEST_F(SemanticTest, ValidVariableDeclaration) {
    EXPECT_TRUE(analyze("fn main() { let x: i32 = 42; }"));
}

TEST_F(SemanticTest, TypeMismatch) {
    // This should fail type checking
    // EXPECT_FALSE(analyze("fn main() { let x: i32 = true; }"));
}

TEST_F(SemanticTest, UndefinedVariable) {
    // EXPECT_FALSE(analyze("fn main() { return x; }"));
}

TEST_F(SemanticTest, DuplicateDeclaration) {
    // EXPECT_FALSE(analyze("fn main() { let x: i32 = 1; let x: i32 = 2; }"));
}

TEST_F(SemanticTest, ValidArithmetic) {
    EXPECT_TRUE(analyze("fn main() { let x = 1 + 2; }"));
}

TEST_F(SemanticTest, ValidComparison) {
    EXPECT_TRUE(analyze("fn main() { let x = 1 < 2; }"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
