cmake_minimum_required(VERSION 3.16)
project(MyMemoryPool C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_library(mempool STATIC mempool.c mempool.h)
target_include_directories(mempool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_mempool test_mempool.c)
target_link_libraries(test_mempool PRIVATE mempool)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME test_mempool COMMAND test_mempool)
endif()
