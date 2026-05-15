# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Test executables
add_executable(test_lexer unit/test_lexer.cpp)
target_link_libraries(test_lexer compiler_lib gtest_main)

add_executable(test_parser unit/test_parser.cpp)
target_link_libraries(test_parser compiler_lib gtest_main)

add_executable(test_semantic unit/test_semantic.cpp)
target_link_libraries(test_semantic compiler_lib gtest_main)

add_executable(test_pipeline integration/test_full_pipeline.cpp)
target_link_libraries(test_pipeline compiler_lib gtest_main)

# Register tests
include(GoogleTest)
gtest_discover_tests(test_lexer)
gtest_discover_tests(test_parser)
gtest_discover_tests(test_semantic)
gtest_discover_tests(test_pipeline)
