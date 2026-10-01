#pragma once

#include "core/camera.h"
#include "renderer/framebuffer.h"
#include "renderer/shader.h"
#include "renderer/shadowmap.h"
#include "scene/scene.h"

class Renderer {
public:
    enum PostEffect { None, Grayscale, Invert, Blur, Sharpen, Edges };

    bool blinn = true;
    bool wireframe = false;
    bool faceCulling = true;
    bool showSkybox = true;
    bool reflections = true;
    bool fresnel = true;
    int postEffect = None;
    float gamma = 1.0f;

    // Sun shadows.
    bool shadows = true;
    int shadowResolution = 2048;
    float shadowBiasMin = 0.0005f;
    float shadowBiasMax = 0.005f;
    bool pcf = true;
    bool shadowCullFront = false; // only back faces go into the shadow map, an alternative fix for acne

    Renderer(int width, int height);
    ~Renderer();

    // Draws a frame: the scene into the off-screen target, then the post pass onto the window.
    void render(Scene& scene, const Camera& camera, int width, int height);

    // For the debug preview.
    GLuint shadowMapTexture() const { return shadowMap.textureId(); }

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

private:
    void drawLit(const Scene& scene, const Mesh& mesh, const Material& material, const glm::mat4& model);

    Shader lit;
    Shader lamp;
    Shader post;
    Shader depth;
    Shader skybox;
    Framebuffer sceneTarget;
    ShadowMap shadowMap;
    GLuint emptyVao = 0;
};
