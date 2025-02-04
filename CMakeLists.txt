cmake_minimum_required(VERSION 3.22)
project(TelltaleChimera)

set(CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(UNIX)
    set(TTC_TARGET_ARCH unix)
    if(APPLE)
        set(TTC_TARGET_UNIX macos)
    else()
        set(TTC_TARGET_UNIX linux)
    endif()
else()
    set(TTC_TARGET_ARCH win32)
endif()


add_executable(chimera)
add_subdirectory(src)
add_subdirectory(extern)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(chimera PRIVATE DEBUG=1)
else()
    target_compile_definitions(chimera PRIVATE RELEASE=1)
endif()



target_compile_options(chimera PRIVATE -Wall)
target_include_directories(chimera PRIVATE include)