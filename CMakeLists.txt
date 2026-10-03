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

# GLM: header-only math library (vectors, matrices, transforms). Graphics-API agnostic.
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glm)

# Dear ImGui: immediate-mode UI for debug panels. No CMake project, so its sources are built as a static lib,
# together with the GLFW (input) and OpenGL 3+ (rendering) backends. The GL backend has its own loader, no GLAD needed.
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

# Assimp: 3D model importer. Only the glTF and OBJ importers, to keep the (large) build smaller.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.5
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(assimp)

# EnTT: header-only entity component system.
FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.16.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(entt)

# nlohmann/json: JSON for scene files. Header-only.
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(json)

# GLAD: OpenGL 4.6 core function loader, pre-generated and vendored in external/glad.
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# The engine: a static library that knows nothing about any game. Its include root is src/engine, so
# project headers are included by folder, e.g. "renderer/mesh.h".
add_library(dgfx_engine STATIC
    src/engine/assets/assets.cpp
    src/engine/assets/model.cpp
    src/engine/core/application.cpp
    src/engine/core/input.cpp
    src/engine/core/window.cpp
    src/engine/renderer/cubemap.cpp
    src/engine/renderer/framebuffer.cpp
    src/engine/renderer/light.cpp
    src/engine/renderer/mesh.cpp
    src/engine/renderer/renderer.cpp
    src/engine/renderer/shader.cpp
    src/engine/renderer/shadowmap.cpp
    src/engine/renderer/texture.cpp
    src/engine/scene/component_registry.cpp
    src/engine/scene/scene.cpp
    src/engine/scene/scene_io.cpp
    src/engine/ui/display_panel.cpp
    src/engine/ui/renderer_panel.cpp
    src/engine/ui/scene_panels.cpp
    src/engine/ui/ui.cpp)
target_link_libraries(dgfx_engine PUBLIC glfw glad glm::glm imgui assimp::assimp EnTT::EnTT nlohmann_json::nlohmann_json)
target_include_directories(dgfx_engine PUBLIC src/engine PRIVATE ${stb_SOURCE_DIR})

# Shaders, textures, models and scenes are loaded from the source tree, so the exe finds them regardless of its working directory.
# ponytail: absolute source path, copy them next to the exe if it ever ships without the repo.
target_compile_definitions(dgfx_engine PRIVATE
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/"
    TEXTURE_DIR="${CMAKE_SOURCE_DIR}/textures/"
    MODEL_DIR="${CMAKE_SOURCE_DIR}/models/"
    SCENE_DIR="${CMAKE_SOURCE_DIR}/scenes/")

# The game: an executable on top of the engine. src/ is its include root, for "game/game.h".
add_executable(dgfx
    src/main.cpp
    src/game/components.cpp
    src/game/demo.cpp
    src/game/game.cpp
    src/game/game_app.cpp)
target_link_libraries(dgfx PRIVATE dgfx_engine)
target_include_directories(dgfx PRIVATE src)
