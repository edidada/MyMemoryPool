cmake_minimum_required(VERSION 3.16)
project(MyMemoryPool VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)

# Build the memory pool as a static library. vcpkg ports generally consume the
# static variant; we keep it static-only to avoid platform-specific DLL export
# plumbing for this small header + single C file library.
add_library(mempool STATIC mempool.c mempool.h)
target_include_directories(mempool PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(mempool PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER mempool.h)
if(MSVC)
    target_compile_options(mempool PRIVATE /W4)
else()
    target_compile_options(mempool PRIVATE -Wall -Wextra)
endif()

# ---- Install rules + CMake package config so consumers can `find_package(mempool)` ----
install(TARGETS mempool
    EXPORT mempool-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT mempool-targets
    FILE mempool-config.cmake
    NAMESPACE mempool::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mempool)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_mempool test_mempool.c)
    target_link_libraries(test_mempool PRIVATE mempool)
    set_target_properties(test_mempool PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    add_test(NAME mempool_correctness_and_stress COMMAND test_mempool)
    set_tests_properties(mempool_correctness_and_stress PROPERTIES
        PASS_REGULAR_EXPRESSION "ALL TESTS PASSED" TIMEOUT 120)
endif()
