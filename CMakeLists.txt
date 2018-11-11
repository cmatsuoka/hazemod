cmake_minimum_required(VERSION 3.3)
project(hazemod)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_RELEASE 1)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE})

option(BUILD_TESTS "Build tests" ON)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
    add_test(NAME full COMMAND test_hazemod)
endif()
