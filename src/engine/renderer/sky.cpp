#include "renderer/sky.h"

#include "core/paths.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

// The same atmosphere as sky.frag.
static constexpr float kEarthRadius = 6360e3f;
static constexpr float kAtmosphereRadius = 6420e3f;
static constexpr float kRayleighHeight = 7994.0f;
static constexpr float kMieHeight = 1200.0f;
static const glm::vec3 kRayleighBeta{5.8e-6f, 13.5e-6f, 33.1e-6f};
static constexpr float kMieBeta = 21e-6f;

ProceduralSky::ProceduralSky() : shader(assetRoot() + "shaders/skybox.vert", assetRoot() + "shaders/sky.frag") {
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

    // Half floats: the sky is HDR. All mip levels, so the smallest one can give the sky's average color.
    const int levels = static_cast<int>(std::log2(kSize)) + 1;
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, levels, GL_RGBA16F, kSize, kSize);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // sample the full size only
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &fbo);
}

ProceduralSky::~ProceduralSky() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &cubemap);
}

void ProceduralSky::bind(GLuint unit) const {
    glBindTextureUnit(unit, cubemap);
}

void ProceduralSky::update(const Sky& sky, glm::vec3 toSun, const Mesh& cube) {
    if (drawn && sky == lastSky && toSun == lastToSun)
        return;

    drawn = true;
    lastSky = sky;
    lastToSun = toSun;

    // A camera at the center that looks at each face in turn, with a 90 degree view: the faces' fixed GL order
    // and orientation (+X, -X, +Y, -Y, +Z, -Z).
    const glm::vec3 forward[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 up[6] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
    const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    shader.use();
    shader.setMat4("projection", projection);
    shader.setVec3("toSun", toSun);
    shader.setFloat("sunIntensity", sky.sunIntensity);
    shader.setFloat("rayleighScale", sky.rayleigh);
    shader.setFloat("mieScale", sky.mie);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, kSize, kSize);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // inside the cube

    for (int face = 0; face < 6; face++) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, cubemap, 0);
        shader.setMat4("view", glm::lookAt(glm::vec3(0.0f), forward[face], up[face]));
        cube.draw();
    }

    // The 1x1 level of the top face is the average of the sky above 45 degrees.
    // ponytail: reading back makes the CPU wait for the GPU, once per change. Fine while the sun is moved by hand;
    // for a sun that moves every frame, keep the ambient on the GPU.
    glGenerateTextureMipmap(cubemap);
    const int top = static_cast<int>(std::log2(kSize));
    float average[4] = {};
    glGetTextureSubImage(cubemap, top, 0, 0, 2, 1, 1, 1, GL_RGBA, GL_FLOAT, sizeof(average), average); // z 2 = +Y face
    ambientColor = glm::vec3(average[0], average[1], average[2]);
}

// The air between the point at `height` meters and the top of the atmosphere along `dir`, weighted by density.
static glm::vec2 opticalDepth(float height, glm::vec3 dir) {
    const glm::vec3 origin(0.0f, kEarthRadius + height, 0.0f);

    // Below the horizon the ray hits the planet: no light.
    const float b = glm::dot(origin, dir);
    const float cEarth = glm::dot(origin, origin) - kEarthRadius * kEarthRadius;

    if (b < 0.0f && b * b - cEarth > 0.0f)
        return glm::vec2(INFINITY);

    const float cAtmosphere = glm::dot(origin, origin) - kAtmosphereRadius * kAtmosphereRadius;
    const float far = -b + std::sqrt(b * b - cAtmosphere);
    const int steps = 32;
    const float step = far / steps;
    glm::vec2 depth(0.0f);

    for (int i = 0; i < steps; i++) {
        const float h = glm::length(origin + dir * ((i + 0.5f) * step)) - kEarthRadius;
        depth += glm::vec2(std::exp(-h / kRayleighHeight), std::exp(-h / kMieHeight)) * step;
    }

    return depth;
}

glm::vec3 ProceduralSky::sunlight(const Sky& sky, glm::vec3 toSun) {
    const glm::vec2 depth = opticalDepth(1.0f, toSun);

    if (std::isinf(depth.x))
        return glm::vec3(0.0f);

    const glm::vec3 tau = kRayleighBeta * sky.rayleigh * depth.x + glm::vec3(kMieBeta * sky.mie * 1.1f * depth.y);
    return glm::exp(-tau) * sky.sunIntensity;
}

glm::vec3 ProceduralSky::sunColor(const Sky& sky, glm::vec3 toSun) {
    return sunlight(sky, toSun) / sunlight(sky, glm::vec3(0.0f, 1.0f, 0.0f));
}
