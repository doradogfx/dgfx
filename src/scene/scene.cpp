#include "scene/scene.h"

#include <glm/gtc/matrix_transform.hpp>

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

static PointLight coloredLight(glm::vec3 color) {
    PointLight light;
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
      sky(TEXTURE_DIR "skybox/"),
      shiba(MODEL_DIR "shiba/scene.gltf", textures) {
    // Gold, from the classic OpenGL material tables (devernay.free.fr/cours/opengl/materials.html).
    // Unlike plastic, a metal's highlight takes the metal's own color.
    const Material gold = {nullptr, nullptr, srgb({0.75164f, 0.60648f, 0.22648f}), srgb({0.628281f, 0.555802f, 0.366065f}), 51.2f * 4.0f, 0.4f}; // table value is for Phong

    // Wooden crate with a steel rim. The specular map is black over the wood and bright over the metal,
    // so only the rim catches highlights, and only the rim reflects the sky. White tints: the maps decide.
    const Material crate = {&crateDiffuse, &crateSpecular, glm::vec3(1.0f), glm::vec3(1.0f), 128.0f, 0.3f};

    // Chrome: almost no color of its own, a perfect mirror of the environment.
    const Material chrome = {nullptr, nullptr, glm::vec3(0.02f), glm::vec3(1.0f), 512.0f, 1.0f};

    // Glass: mostly see-through. 0.04 is real glass's head-on reflectivity; Fresnel raises it at the edges.
    const Material glass = {nullptr, nullptr, glm::vec3(0.02f), glm::vec3(1.0f), 512.0f, 0.04f, 0.9f, 1.52f};

    auto addMesh = [&](const char* name, const Mesh& mesh, const Material& material, const Transform& transform) {
        registry.emplace<MeshRenderer>(create(name, transform), &mesh, material);
    };

    addMesh("Floor", cube, rubber({0.6f, 0.6f, 0.6f}), {.position = {0.0f, -0.55f, 0.0f}, .scale = {10.0f, 0.1f, 10.0f}}); // top surface at y = -0.5
    addMesh("Orange sphere", sphere, plastic({1.0f, 0.5f, 0.3f}), {.position = {0.0f, 0.0f, 0.0f}});
    addMesh("Blue sphere", sphere, plastic({0.3f, 0.5f, 1.0f}, 512.0f), {.position = {2.0f, 0.0f, -1.0f}, .rotation = {0.0f, 30.0f, 0.0f}});
    addMesh("Green sphere", sphere, rubber({0.4f, 0.9f, 0.4f}), {.position = {-1.5f, -0.25f, 1.0f}, .scale = glm::vec3(0.5f)});
    addMesh("Purple sphere", sphere, plastic({0.7f, 0.05f, 0.87f}), {.position = {1.5f, 0.0f, 1.0f}, .rotation = {0.0f, 45.0f, 0.0f}});
    addMesh("Gold sphere", sphere, gold, {.position = {-2.2f, 0.0f, -1.2f}, .rotation = {0.0f, 15.0f, 0.0f}});
    addMesh("Crate", cube, crate, {.position = {0.0f, 0.0f, 2.4f}, .rotation = {0.0f, 25.0f, 0.0f}});
    addMesh("Crate", cube, crate, {.position = {-3.2f, 0.0f, 0.6f}, .rotation = {0.0f, 10.0f, 0.0f}});
    addMesh("Chrome sphere", sphere, chrome, {.position = {3.3f, 0.0f, 0.8f}});
    addMesh("Glass sphere", sphere, glass, {.position = {-0.4f, 0.0f, -2.4f}});

    // "Shiba" by zixisun02, CC-BY-4.0 (see models/shiba/license.txt).
    const entt::entity dog = create("Shiba", {.position = {3.6f, -0.5f, -1.6f}, .rotation = {0.0f, -30.0f, 0.0f}, .scale = glm::vec3(shiba.fitScale(1.2f))});
    registry.emplace<ModelRenderer>(dog, &shiba);

    DirLight sun;
    sun.ambient = glm::vec3(0.01f);
    sun.direction = {-0.6f, -1.0f, -0.4f}; // angled, so shadows are long enough to see
    sun.diffuse = srgb({0.45f, 0.42f, 0.38f}); // slightly warm, still dim enough for the point lights to stand out
    registry.emplace<DirLight>(create("Sun"), sun);

    // The point lights circle the scene because their parent spins, not because the renderer moves them.
    const entt::entity orbit = create("Light orbit");
    registry.emplace<Rotator>(orbit, glm::vec3(0.0f, glm::degrees(0.5f), 0.0f));

    registry.emplace<PointLight>(create("Red light", {.position = {2.0f, 1.0f, 1.5f}, .parent = orbit}), coloredLight({1.0f, 0.2f, 0.2f}));
    registry.emplace<PointLight>(create("Green light", {.position = {-2.0f, 1.0f, 1.5f}, .parent = orbit}), coloredLight({0.2f, 1.0f, 0.2f}));
    registry.emplace<PointLight>(create("Blue light", {.position = {-1.5f, 1.2f, -2.0f}, .parent = orbit}), coloredLight({0.2f, 0.3f, 1.0f}));
    registry.emplace<PointLight>(create("White light", {.position = {2.0f, 1.5f, -2.0f}, .parent = orbit}), coloredLight({1.0f, 1.0f, 1.0f}));

    registry.emplace<SpotLight>(create("Flashlight")); // follows the camera, off by default
}

entt::entity Scene::create(const std::string& name, const Transform& transform) {
    const entt::entity entity = registry.create();
    registry.emplace<Name>(entity, name);
    registry.emplace<Transform>(entity, transform);
    return entity;
}

glm::mat4 Scene::worldMatrix(entt::entity entity) const {
    const Transform& t = registry.get<Transform>(entity);

    // Right to left: scale, then rotate (roll, pitch, yaw), then move into place.
    glm::mat4 local = glm::translate(glm::mat4(1.0f), t.position);
    local = glm::rotate(local, glm::radians(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    local = glm::rotate(local, glm::radians(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    local = glm::rotate(local, glm::radians(t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    local = glm::scale(local, t.scale);

    // ponytail: walks the parent chain on every call, cache world matrices with dirty flags if scenes get large.
    return t.parent == entt::null ? local : worldMatrix(t.parent) * local;
}

void Scene::update(float dt) {
    for (auto [entity, rotator, transform] : registry.view<Rotator, Transform>().each()) {
        if (rotator.enabled)
            transform.rotation = glm::mod(transform.rotation + rotator.degreesPerSecond * dt, glm::vec3(360.0f));
    }
}
