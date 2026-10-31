cmake_minimum_required(VERSION 3.16)
project(MyMemoryPool C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(MEMPOOL_SOURCES mempool.c mempool.h)

# The library that gets packaged (installed) by `conan create`.
add_library(mempool STATIC ${MEMPOOL_SOURCES})
target_include_directories(mempool PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_compile_features(mempool PUBLIC c_std_11)

install(TARGETS mempool
        EXPORT mempool-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
install(FILES mempool.h DESTINATION include)

# Export a CMake config so the installed tree is also find_package-able.
install(EXPORT mempool-targets
        NAMESPACE mempool::
        FILE mempool-targets.cmake
        DESTINATION lib/cmake/mempool)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/mempool-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mempool-config.cmake
    INSTALL_DESTINATION lib/cmake/mempool)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mempool-config-version.cmake
    VERSION 0.1.0
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mempool-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/mempool-config-version.cmake
    DESTINATION lib/cmake/mempool)

# In-tree test program (NOT installed into the package).
add_executable(test_mempool test_mempool.c)
target_link_libraries(test_mempool PRIVATE mempool)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME test_mempool COMMAND test_mempool)
endif()
