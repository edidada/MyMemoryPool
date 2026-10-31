cmake_minimum_required(VERSION 3.16)
project(MyMemoryPool VERSION 1.0.0 LANGUAGES C)

add_library(mempool STATIC mempool.c)
target_include_directories(mempool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(mempool PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)

if(MSVC)
    target_compile_options(mempool PRIVATE /W4)
else()
    target_compile_options(mempool PRIVATE -Wall -Wextra)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(test_mempool test_mempool.c)
    target_link_libraries(test_mempool PRIVATE mempool)
    set_target_properties(test_mempool PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    add_test(NAME mempool_correctness_and_stress COMMAND test_mempool)
    set_tests_properties(mempool_correctness_and_stress PROPERTIES
        PASS_REGULAR_EXPRESSION "ALL TESTS PASSED" TIMEOUT 120)
endif()
