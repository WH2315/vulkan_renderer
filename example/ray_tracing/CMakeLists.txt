project(ray_tracing)

file(GLOB_RECURSE source CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/*.hpp)

add_executable(${PROJECT_NAME} ${source} ${headers})

target_link_libraries(${PROJECT_NAME} PRIVATE renderer)
target_include_directories(${PROJECT_NAME} PRIVATE include)