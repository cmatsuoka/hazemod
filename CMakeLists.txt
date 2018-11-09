cmake_minimum_required(VERSION 2.8.9)
project(hazemod)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_RELEASE 1)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE})

option(BUILD_TESTS "Build tests" OFF)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
    add_test(NAME full COMMAND test_hazemod)
endif()
