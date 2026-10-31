from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout


class MyMemoryPoolConan(ConanFile):
    name = "mempool"
    version = "0.1.0"
    description = "A small toy C memory pool (free-list allocator with size classes)"
    homepage = "https://github.com/edidada/MyMemoryPool"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "mempool.h", "mempool.c", "test_mempool.c", "mempool-config.cmake.in"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["mempool"]
        self.cpp_info.set_property("cmake_target_name", "mempool::mempool")
        self.cpp_info.set_property("cmake_file_name", "mempool")
