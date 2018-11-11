cmake_minimum_required(VERSION 2.8.9)

include_directories(.)

add_library(hazemod SHARED
    util/util.cpp
    mixer/channel.cpp
    mixer/sample.cpp
    mixer/mixer.cpp
    player/pt21a.cpp
)

set_target_properties(hazemod PROPERTIES
    VERSION ${VERSION}
    SOVERSION ${VERSION_MAJOR}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)
