cmake_minimum_required(VERSION 3.3)
project(pa-example)
include_directories(../../include)
link_directories(../../lib)
add_executable(play-portaudio main.cpp)
target_link_libraries(play-portaudio portaudio hazemod)
