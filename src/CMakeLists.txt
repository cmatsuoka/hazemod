cmake_minimum_required(VERSION 2.8.9)

add_library(hazemod SHARED
    mixer/channel.cpp
    mixer/sample.cpp
    mixer/mixer.cpp
)

set_target_properties(hazemod PROPERTIES
    VERSION ${VERSION}
    SOVERSION ${VERSION_MAJOR}
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)
