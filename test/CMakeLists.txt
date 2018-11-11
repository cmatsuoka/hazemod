include_directories(../src)

add_executable(test_hazemod
    test_util_util.cpp
    test_util_databuffer.cpp
    test_mixer_nearest.cpp
    test_mixer_linear.cpp
    test.cpp)

target_compile_definitions(test_hazemod PRIVATE TOP_DIR="${PROJECT_SOURCE_DIR}")

