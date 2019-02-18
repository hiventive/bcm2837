from conans import ConanFile, CMake, tools
from conans.util import files
import yaml


class Bcm2837Conan(ConanFile):
    name = "bcm2837"
    version = str(yaml.load(tools.load("settings.yml"))['project']['version'])
    license = "MIT"
    url = "git@git.hiventive.com:socs/broadcom/bcm2837.git"
    description = "BCM2837"
    settings = "cppstd", "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [
        True, False], "fPIE": [True, False]}
    default_options = "shared=False", "fPIC=False", "fPIE=False"
    generators = "cmake"
    exports = "settings.yml"
    exports_sources = "src/*", "CMakeLists.txt"
    requires = "pl011/0.1.0@hiventive/testing", \
               "bcm2836-control/0.1.0@hiventive/testing", \
               "bcm2835-armctrl-ic/0.1.0@hiventive/testing", \
               "memory/0.1.0@hiventive/testing", \
               "bcm2835-gpio/0.1.0@hiventive/testing", \
               "uart-ws/0.1.0@hiventive/testing", \
               "qmg2sc/0.1.0@hiventive/testing"

    def _configure_cmake(self):
        cmake = CMake(self)
        if self.settings.os != "Windows":
            cmake.definitions["CMAKE_POSITION_INDEPENDENT_CODE"] = self.options.fPIC or self.options.fPIE
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("*.h", dst="include", src="src")
        self.copy("*.hpp", dst="include", src="src")

    def package_info(self):
        self.cpp_info.libs = ["bcm2837"]
