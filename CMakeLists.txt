cmake_minimum_required(VERSION 3.7)
project(PolymorphismTests)

include_directories(include)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
add_definitions(-fconcepts -O0)
set(CMAKE_CXX_STANDARD 17)

add_executable(tests test.cpp)
target_link_libraries(tests ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
