project(PackageTest CXX)
enable_language(C) # Fix CMake 3.1.0 (at least) thread issue
cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 11)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(SystemCLanguage CONFIG REQUIRED 2.3.2)

add_executable(example example.cpp)
target_link_libraries(example bcm2837 ${CONAN_LIBS})

enable_testing()
add_test(NAME example
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
		COMMAND example)
