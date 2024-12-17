cmake_minimum_required(VERSION 3.22)
project(TelltaleChimera)

set(CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(chimera)
add_subdirectory(src)
add_subdirectory(extern)

target_compile_options(chimera PRIVATE -Wall)
target_include_directories(chimera PUBLIC include)