from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class MyMemoryPoolConan(ConanFile):
    name = "mymemorypool"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
