#pragma once

#include "renderer/mesh.h"
#include "renderer/shader.h"
#include "scene/components.h"

#include <glm/glm.hpp>

// The Sky component drawn into a cubemap. The scattering is costly, so the cubemap is drawn again only when the
// sun or the settings change. The skybox and the reflections then read it like a sky texture.
class ProceduralSky {
public:
    ProceduralSky();
    ~ProceduralSky();

    // Draws the cubemap again if the sun or the settings changed since the last time. Changes the framebuffer,
    // the viewport and the culling.
    void update(const Sky& sky, glm::vec3 toSun, const Mesh& cube);

    // Draw again on the next update, e.g. after the shader was reloaded.
    void invalidate() { drawn = false; }

    void bind(GLuint unit) const;

    // The average color of the upper sky, from the last update: the light that the sky gives the objects.
    glm::vec3 ambient() const { return ambientColor; }

    // The sunlight that reaches the ground: the sun's light after the air on the way. Black below the horizon.
    static glm::vec3 sunlight(const Sky& sky, glm::vec3 toSun);

    // The same, as a color that is white when the sun is straight up: for the sun's DirLight.
    static glm::vec3 sunColor(const Sky& sky, glm::vec3 toSun);

    Shader shader; // public for hot reload and the shader list

    ProceduralSky(const ProceduralSky&) = delete;
    ProceduralSky& operator=(const ProceduralSky&) = delete;

private:
    static constexpr int kSize = 256; // the scattering changes slowly across the sky, so this looks sharp

    GLuint cubemap = 0;
    GLuint fbo = 0;
    bool drawn = false;
    Sky lastSky;
    glm::vec3 lastToSun{0.0f};
    glm::vec3 ambientColor{0.0f};
};
