cmake_minimum_required(VERSION 3.3)
project(simple)
include_directories(../../include)
link_directories(../../lib)
add_executable(play main.cpp)
target_link_libraries(play hazemod)
