# Tests

file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${CMAKE_PROJECT_NAME_LOWER}-exec ${TEST_FILES})
target_compile_definitions(${CMAKE_PROJECT_NAME_LOWER}-exec PRIVATE "QBOXCPU_LIBDIR=\"${CONAN_LIB_DIRS_QBOXCPU}\"" "RASPI3_TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
target_include_directories(${CMAKE_PROJECT_NAME_LOWER}-exec PUBLIC ${GTEST_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

set(${CMAKE_PROJECT_NAME_LOWER}-LIBRARIES
    ${GTEST_LIBRARIES}
    ${CMAKE_PROJECT_NAME_LOWER}
    qmg2sc
    qmg
    qemu-system-aarch64
    uart-ws
    boost_system
    pl011
    bcm2836-control
    bcm2835-gpio
    genericgpio
    bcm2835-armctrl-ic
    register
    communication
    module
    configuration
    common
    cciapi
    SystemC::systemc
    ${YAML_CPP_LIBRARIES}
    ${GLIB_LIBRARIES})

# Disable SPDLog with VS 2012
if(NOT (MSVC AND MSVC_VERSION EQUAL 1700))
	list(APPEND ${CMAKE_PROJECT_NAME_LOWER}-LIBRARIES spdlog::spdlog)
endif()

if(NOT MSVC)
	list(APPEND ${CMAKE_PROJECT_NAME_LOWER}-LIBRARIES Threads::Threads)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME_LOWER}-exec ${${CMAKE_PROJECT_NAME_LOWER}-LIBRARIES})

add_test(NAME ${CMAKE_PROJECT_NAME}Tests COMMAND ${CMAKE_PROJECT_NAME_LOWER}-exec)

# # RPi copy
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/RPi DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
