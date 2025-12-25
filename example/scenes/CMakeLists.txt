project(scenes)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/*.hpp)

add_executable(scenes ${sources} ${headers})

target_include_directories(scenes PRIVATE include)
target_link_libraries(scenes PRIVATE renderer)