# dgfx

A game engine written in C++ as a learning project: the goal is to understand how game engines are designed and implemented by building one step by step, one feature at a time.

## External libraries

- [GLFW](https://www.glfw.org/): cross-platform window creation, OpenGL context and input handling.
- [GLAD](https://github.com/Dav1dde/glad): OpenGL function loader (OpenGL 4.6 core), generated with the [GLAD web generator](https://glad.dav1d.de/).
- [stb_image](https://github.com/nothings/stb): single-header image loader (PNG, JPG, ...) used to load textures.
- [GLM](https://github.com/g-truc/glm): header-only math library for vectors, matrices and transformations.

## Building

Requires CMake 3.20+ and a C++20 compiler.

```
cmake -S . -B build
cmake --build build --config Debug
```
