cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

project(
    vulkan_renderer
    LANGUAGES CXX C
)

find_package(Vulkan REQUIRED)

add_subdirectory(3rdlibs)
add_subdirectory(renderer)

option(BUILD_SANDBOX "build sandbox" ON)
if (BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

option(BUILD_EXAMPLE "build example" ON)
if (BUILD_EXAMPLE)
    add_subdirectory(example)
endif()