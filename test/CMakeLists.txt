include_directories(../src)

add_executable(test_hazemod
    test_util_util.cpp
    test_util_state.cpp
    test_util_databuffer.cpp
    test_mixer_nearest.cpp
    test_mixer_linear.cpp
    test_mixer_spline.cpp
    test_mixer_sample.cpp
    test_mixer_softmixer_voice.cpp
    test_mixer_softmixer.cpp
    test_mixer_paula_channel.cpp
    test_format_format.cpp
    test_format_mod_fingerprint.cpp
    test_format_st_fingerprint.cpp
    test.cpp)

target_compile_definitions(test_hazemod PRIVATE TEST_DIR="${PROJECT_SOURCE_DIR}/test")

