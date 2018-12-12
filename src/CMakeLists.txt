cmake_minimum_required(VERSION 2.8.9)

include_directories(.)

if(PROFILE)
    if(CMAKE_COMPILER_IS_GNUCC OR (CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    endif()
endif()

add_definitions(-DHAZE_SYM_EXPORT)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

add_library(hazemod SHARED
    util/util.cpp
    format/format.cpp
    format/format_registry.cpp
    format/mod.cpp
    format/stm.cpp
    format/s3m.cpp
    format/fest.cpp
    format/st.cpp
    mixer/spline.cpp
    mixer/sample.cpp
    mixer/softmixer_voice.cpp
    mixer/softmixer.cpp
    mixer/paula.cpp
    player/scanner.cpp
    player/player.cpp
    player/player_registry.cpp
    player/pt21a.cpp
    player/nt11.cpp
    player/dst2.cpp
    player/ust.cpp
    player/hmn.cpp
    player/st2play/st2play.c
    player/st2.cpp
    player/st3play/st3play.cpp
    player/st3play/load_s3m.cpp
    player/st3play/load_mod.cpp
    player/st3.cpp
    player/ft2play/ft2play.cpp
    player/ft2.cpp
    player/ft101.cpp
    haze.cpp
)

set_target_properties(hazemod PROPERTIES
    VERSION ${VERSION}
    SOVERSION ${VERSION_MAJOR}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

if(MSVC)
    target_compile_options(hazemod PRIVATE /W4)
else(MSVC)
    target_compile_options(hazemod PRIVATE -Wall)
endif(MSVC)

