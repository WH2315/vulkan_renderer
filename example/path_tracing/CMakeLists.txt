project(path_tracing)

file(GLOB_RECURSE source CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/*.hpp)

add_executable(path_tracing ${source} ${headers})
target_include_directories(path_tracing PRIVATE include)
target_link_libraries(path_tracing PRIVATE renderer)