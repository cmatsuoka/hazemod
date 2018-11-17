cmake_minimum_required(VERSION 2.8.9)

include_directories(.)

#add_definitions(-DHAZE_SYM_EXPORT)
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)

add_library(hazemod SHARED
    util/util.cpp
    format/mod.cpp
    mixer/voice.cpp
    mixer/sample.cpp
    mixer/mixer.cpp
    player/pt21a.cpp
    player/nt11.cpp
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

