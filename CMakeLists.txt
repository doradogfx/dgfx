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

# stb_image: single-header image decoder (PNG/JPG/...). No CMake project, only its include dir is used.
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20)
FetchContent_MakeAvailable(stb)

# GLAD: OpenGL 4.6 core function loader, pre-generated and vendored in external/glad.
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(dgfx src/main.cpp src/shader.cpp src/texture.cpp)
target_link_libraries(dgfx PRIVATE glfw glad)
target_include_directories(dgfx PRIVATE ${stb_SOURCE_DIR})

# Shaders and textures are loaded from the source tree, so the exe finds them regardless of its working directory.
# ponytail: absolute source path, copy them next to the exe if it ever ships without the repo.
target_compile_definitions(dgfx PRIVATE
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/"
    TEXTURE_DIR="${CMAKE_SOURCE_DIR}/textures/")
