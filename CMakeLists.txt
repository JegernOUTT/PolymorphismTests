cmake_minimum_required(VERSION 3.7)
project(PolymorphismTests)

add_definitions(-fconcepts -O0)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main_static_polyformic.cpp)
add_executable(untitled ${SOURCE_FILES})