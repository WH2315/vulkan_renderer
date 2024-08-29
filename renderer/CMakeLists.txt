project(renderer)

file(GLOB_RECURSE SRC "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SRC} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./include ../3rdlibs/stb)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog glfw Vulkan::Vulkan glm VulkanMemoryAllocator imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE glslang glslang::SPIRV glslang::glslang-default-resource-limits)