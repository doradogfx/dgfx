cmake_minimum_required(VERSION 3.20)
project(dgfx LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW: window, OpenGL context and input. Fetched and built from source at configure time.
include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glfw)

# GLAD: OpenGL 4.6 core function loader, pre-generated and vendored in external/glad.
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(dgfx src/main.cpp src/shader.cpp)
target_link_libraries(dgfx PRIVATE glfw glad)

# Shaders are loaded from the source tree, so the exe finds them regardless of its working directory.
# ponytail: absolute source path, copy shaders next to the exe if it ever ships without the repo.
target_compile_definitions(dgfx PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/")
