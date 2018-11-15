include_directories(../src)

add_executable(test_hazemod
    test_util_util.cpp
    test_util_databuffer.cpp
    test_mixer_nearest.cpp
    test_mixer_linear.cpp
    test_mixer_sample.cpp
    test_mixer_voice.cpp
    test_mixer_mixer.cpp
    test_format_mod_fingerprint.cpp
    test.cpp)

target_compile_definitions(test_hazemod PRIVATE TEST_DIR="${PROJECT_SOURCE_DIR}/test")

