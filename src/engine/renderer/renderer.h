#pragma once

#include "core/camera.h"
#include "renderer/framebuffer.h"
#include "renderer/mesh.h"
#include "renderer/shader.h"
#include "renderer/shadowmap.h"
#include "renderer/texture.h"
#include "scene/scene.h"

#include <glm/glm.hpp>

#include <vector>

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

    int drawCalls = 0; // draw calls with the lit and lamp shaders in the last frame
    int instances = 0; // objects that those draw calls drew

    // For the debug preview.
    GLuint shadowMapTexture() const { return shadowMap.textureId(); }

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

private:
    // The per-copy data in the instance buffer. The same layout as the Instance struct in lit.vert.
    struct Instance {
        glm::mat4 model;
        glm::mat4 normalMatrix;
    };

    // The copies of one mesh with one material. Its instances are next to each other in the buffer.
    struct Batch {
        const Mesh* mesh;
        Material material;
        GLuint first; // index of the first instance
        std::vector<glm::mat4> models;
    };

    void buildBatches(const Scene& scene);
    void drawBatch(const Batch& batch);

    std::vector<Batch> batches;
    std::vector<Instance> instanceData;
    GLuint instanceBuffer = 0;

    Shader lit;
    Shader lamp;
    Shader post;
    Shader depth;
    Shader skybox;
    Mesh cube;   // the sky
    Mesh sphere; // the point light lamps
    Texture white; // bound wherever a material has no map
    Framebuffer sceneTarget;
    ShadowMap shadowMap;
    GLuint emptyVao = 0;
};
