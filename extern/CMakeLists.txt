
# Why does imgui not have CMakeLists.txt, so annoying
add_subdirectory(hydra)
add_subdirectory(sdl3)
#add_subdirectory(vulkan)
#add_subdirectory(vulkan-headers)

target_link_libraries(chimera hydra)
target_link_libraries(chimera imgui_lib)


if(TRUE)

cmake_minimum_required(VERSION 3.22)
project(imgui_sdl3_vulkan C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# Dear ImGui
set(IMGUI_DIR imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

# Libraries
#find_package(Vulkan REQUIRED)
#find_library(VULKAN_LIBRARY
  #NAMES vulkan vulkan-1)
set(LIBRARIES "SDL3-shared;vulkan")

file(GLOB sources ${IMGUI_DIR}/*.cpp)

add_library(imgui_lib ${sources} ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
target_link_libraries(imgui_lib ${LIBRARIES})
target_include_directories(imgui_lib PUBLIC ${IMGUI_DIR})

endif()












