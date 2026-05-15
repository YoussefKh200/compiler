#include <gtest/gtest.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic_analyzer.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include "optimizer/optimizer.h"
#include "backend/llvm_backend.h"
#include "utils/memory_pool.h"
#include <sstream>

using namespace compiler;

class PipelineTest : public ::testing::Test {
protected:
    void compile(const std::string& code) {
        std::istringstream input(code);
        Lexer lexer(input, "test");
        MemoryPool pool;
        Parser parser(lexer, pool);
        auto ast = parser.parseProgram();
        
        ASSERT_FALSE(parser.hadError());
        
        SemanticAnalyzer sema;
        ASSERT_TRUE(sema.analyze(*ast));
        
        // IR generation would go here
    }
};

TEST_F(PipelineTest, HelloWorld) {
    compile("fn main() { }");
}

TEST_F(PipelineTest, Fibonacci) {
    compile(R"(
        fn fib(n: i32) -> i32 {
            if (n <= 1) {
                return n;
            }
            return fib(n - 1) + fib(n - 2);
        }
    )");
}

TEST_F(PipelineTest, Factorial) {
    compile(R"(
        fn factorial(n: i32) -> i32 {
            let result: i32 = 1;
            let i: i32 = 1;
            while (i <= n) {
                result = result * i;
                i = i + 1;
            }
            return result;
        }
    )");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
