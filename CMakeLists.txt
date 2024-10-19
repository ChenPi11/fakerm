cmake_minimum_required(VERSION 3.10)

project(fakerm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(fakerm ${SOURCES})

install(TARGETS fakerm RUNTIME DESTINATION bin)
