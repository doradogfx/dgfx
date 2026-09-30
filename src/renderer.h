#pragma once

#include "camera.h"
#include "framebuffer.h"
#include "scene.h"
#include "shader.h"

class Renderer {
public:
    enum PostEffect { None, Grayscale, Invert, Blur, Sharpen, Edges };

    bool blinn = true;
    bool wireframe = false;
    int postEffect = None;
    float gamma = 1.0f;

    Renderer(int width, int height);
    ~Renderer();

    // Draws a frame: the scene into the off-screen target, then the post pass onto the window.
    void render(Scene& scene, const Camera& camera, int width, int height, float time);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

private:
    Shader lit;
    Shader lamp;
    Shader post;
    Framebuffer sceneTarget;
    GLuint emptyVao = 0;
};
