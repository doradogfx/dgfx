#include "scene.h"

// Colors picked by eye are sRGB (gamma-encoded); lighting math needs linear values.
// 2.2 approximates the exact sRGB curve closely enough for picked colors.
static glm::vec3 srgb(glm::vec3 color) {
    return glm::pow(color, glm::vec3(2.2f));
}

// Plastic: the surface color everywhere, plus a white-ish highlight (the light's color, not the surface's).
static Material plastic(glm::vec3 color, float shininess = 128.0f) {
    return {nullptr, nullptr, srgb(color), glm::vec3(0.5f), shininess};
}

// Rubber: the surface color, almost no highlight, and what little there is is wide and dull.
static Material rubber(glm::vec3 color) {
    return {nullptr, nullptr, srgb(color), glm::vec3(0.1f), 8.0f};
}

static PointLight coloredLight(glm::vec3 position, glm::vec3 color) {
    PointLight light;
    light.position = position;
    light.ambient = srgb(color) * 0.01f;
    light.diffuse = srgb(color);
    light.specular = srgb(color);
    return light;
}

Scene::Scene()
    : cube(makeCube()),
      sphere(makeSphere()),
      white(glm::vec3(1.0f)),
      crateDiffuse(TEXTURE_DIR "container2.png", true),
      crateSpecular(TEXTURE_DIR "container2_specular.png", false),
      points{
          coloredLight({ 2.0f, 1.0f,  1.5f}, {1.0f, 0.2f, 0.2f}),
          coloredLight({-2.0f, 1.0f,  1.5f}, {0.2f, 1.0f, 0.2f}),
          coloredLight({-1.5f, 1.2f, -2.0f}, {0.2f, 0.3f, 1.0f}),
          coloredLight({ 2.0f, 1.5f, -2.0f}, {1.0f, 1.0f, 1.0f}),
      } {
    sun.ambient = glm::vec3(0.01f);
    sun.direction = {-0.6f, -1.0f, -0.4f}; // angled, so shadows are long enough to see
    sun.diffuse = srgb({0.45f, 0.42f, 0.38f}); // slightly warm, still dim enough for the point lights to stand out

    // Gold, from the classic OpenGL material tables (devernay.free.fr/cours/opengl/materials.html).
    // Unlike plastic, a metal's highlight takes the metal's own color.
    const Material gold = {nullptr, nullptr, srgb({0.75164f, 0.60648f, 0.22648f}), srgb({0.628281f, 0.555802f, 0.366065f}), 51.2f * 4.0f}; // table value is for Phong

    // Wooden crate with a steel rim. The specular map is black over the wood and bright over the metal,
    // so only the rim catches highlights. White tints: the maps alone decide the colors.
    const Material crate = {&crateDiffuse, &crateSpecular, glm::vec3(1.0f), glm::vec3(1.0f), 128.0f};

    objects = {
        {&cube,   { 0.0f, -0.55f,  0.0f},  0.0f, {10.0f, 0.1f, 10.0f}, rubber({0.6f, 0.6f, 0.6f})},            // floor, top surface at y = -0.5
        {&sphere, { 0.0f,  0.0f,   0.0f},  0.0f, { 1.0f, 1.0f,  1.0f}, plastic({1.0f, 0.5f, 0.3f})},           // orange
        {&sphere, { 2.0f,  0.0f,  -1.0f}, 30.0f, { 1.0f, 1.0f,  1.0f}, plastic({0.3f, 0.5f, 1.0f}, 512.0f)},   // blue, glossy
        {&sphere, {-1.5f, -0.25f,  1.0f},  0.0f, { 0.5f, 0.5f,  0.5f}, rubber({0.4f, 0.9f, 0.4f})},            // small green
        {&sphere, { 1.5f,  0.0f,   1.0f}, 45.0f, { 1.0f, 1.0f,  1.0f}, plastic({0.7f, 0.05f, 0.87f})},         // purple
        {&sphere, {-2.2f,  0.0f,  -1.2f}, 15.0f, { 1.0f, 1.0f,  1.0f}, gold},                                  // gold
        {&cube,   { 0.0f,  0.0f,   2.4f}, 25.0f, { 1.0f, 1.0f,  1.0f}, crate},                                 // crate, front
        {&cube,   {-3.2f,  0.0f,   0.6f}, 10.0f, { 1.0f, 1.0f,  1.0f}, crate},                                 // crate, left
    };
}
