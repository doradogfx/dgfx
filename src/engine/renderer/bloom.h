#pragma once

#include "renderer/framebuffer.h"
#include "renderer/shader.h"

#include <vector>

// Glow around bright areas. The HDR scene is filtered down a chain of smaller and smaller textures, then back up,
// adding each level onto the next bigger one. The small levels spread the light wide, the big levels keep it
// close, so the glow fades out smoothly. The post pass mixes the result with the scene.
class Bloom {
public:
    Bloom();
    ~Bloom();

    // Fills the chain from the scene's color and returns the result: a texture at half the scene's size.
    // Uses the full-screen triangle, so a VAO must be bound. Changes the framebuffer and the viewport.
    GLuint apply(const Framebuffer& scene, int width, int height, float radius);

    // Public for hot reload and the shader list in the debug panel.
    Shader down;
    Shader up;

    Bloom(const Bloom&) = delete;
    Bloom& operator=(const Bloom&) = delete;

private:
    static constexpr int kLevels = 6; // 1/2 down to 1/64 of the screen

    struct Level {
        GLuint texture = 0;
        int width = 0;
        int height = 0;
    };

    void resize(int width, int height); // makes the chain again for a new scene size
    void destroy();

    GLuint fbo = 0;
    std::vector<Level> levels;
    int sceneWidth = 0;
    int sceneHeight = 0;
};
