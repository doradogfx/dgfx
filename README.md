# dgfx

![Demo scene rendered by dgfx](docs/images/scene-entities-ui.png)

A game engine written in C++ as a learning project: the goal is to understand how game engines are designed and implemented by building one step by step, one feature at a time.

## Game

A simplified 3D survivors-like built on the engine.

- **Player**: a player that runs relative to the camera, turns to face where it's going, and jumps with gravity.
- **Third-person camera** orbiting the player, with adjustable distance.

## Engine features

- **OpenGL 4.6 core** rendering on a GLFW window, with vsync and resize/minimize handling.
- **Shaders** loaded from files, compiled and linked, with error reporting and typed uniform setters.
- **Meshes** that own their GPU buffers (VAO/VBO/EBO), with procedural cube and UV sphere generation.
- **Textures** loaded with stb_image (PNG/JPG), sRGB or linear, mipmapped with anisotropic filtering, bound to multiple texture units.
- **3D transforms**: model/view/projection matrices, perspective projection, depth testing, back-face culling.
- **Fly camera**: mouse look, WASD movement, zoom, frame-rate independent speed.
- **Lighting**: Blinn-Phong with multiple lights at once: a directional sun, up to 4 point lights (distance attenuation) and a camera flashlight (spot with soft edges). Per-object materials: diffuse/specular colors and shininess, with optional lighting maps (diffuse + specular textures) for per-pixel materials. Environment reflections and refraction from the skybox, with Fresnel. Directional shadow mapping for the sun. Gamma-correct: lighting is computed in linear space (sRGB textures and framebuffer).
- **Post-processing**: the scene renders to an off-screen framebuffer, then a full-screen pass applies effects (grayscale, invert, blur, sharpen, edge detection) and a user gamma adjustment.
- **Skybox**: cubemap environment rendered behind the scene (depth at the far plane, camera translation removed).
- **Model loading** with Assimp (glTF, OBJ): meshes and their materials (diffuse/specular textures, double-sided surfaces), with a texture cache so shared images load once.
- **Entities and components** with EnTT: parent/child transform hierarchy and a scene tree and inspector in the debug panel.
- **Debug UI** with Dear ImGui: separate panels for the scene hierarchy, the selected entity's components, and rendering and display settings, plus a stats overlay.

## Controls

| Input | Action |
|---|---|
| Mouse | Orbit the camera |
| W / A / S / D | Run forward / left / back / right |
| Space | Jump |
| Scroll wheel | Camera distance |
| F1 | Toggle the debug fly camera |
| Tab | Toggle captured mode (cursor hidden) / UI mode (cursor free, to use the panels) |
| Click outside the panels | Back to captured mode (the cursor is also released when the window loses focus) |
| Esc | Quit |

Fly camera: mouse to look, W / A / S / D to move, Space / Left Ctrl up / down, Left Shift faster, scroll wheel to zoom.

## External libraries

- [GLFW](https://www.glfw.org/): cross-platform window creation, OpenGL context and input handling.
- [GLAD](https://github.com/Dav1dde/glad): OpenGL function loader (OpenGL 4.6 core), generated with the [GLAD web generator](https://glad.dav1d.de/).
- [stb_image](https://github.com/nothings/stb): single-header image loader (PNG, JPG, ...) used to load textures.
- [GLM](https://github.com/g-truc/glm): header-only math library for vectors, matrices and transformations.
- [Dear ImGui](https://github.com/ocornut/imgui): immediate-mode UI for the debug panel (GLFW + OpenGL 3 backends).
- [Assimp](https://github.com/assimp/assimp): 3D model importer (glTF and OBJ importers enabled).
- [EnTT](https://github.com/skypjack/entt): header-only entity component system.

## Building

Requires CMake 3.20+ and a C++20 compiler.

```
cmake -S . -B build
cmake --build build --config Debug
```

## Credits

- "Shiba" (https://sketchfab.com/3d-models/shiba-faef9fe5ace445e7b2989d1c1ece361c) by zixisun02 (https://sketchfab.com/zixisun51), licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/).
