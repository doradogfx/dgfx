#include "scene/demo.h"

static PointLight coloredLight(glm::vec3 color) {
    PointLight light;
    light.ambient = srgb(color) * 0.01f;
    light.diffuse = srgb(color);
    light.specular = srgb(color);
    return light;
}

void buildDemo(Scene& scene) {
    entt::registry& registry = scene.registry;

    // Gold, from the classic OpenGL material tables (devernay.free.fr/cours/opengl/materials.html).
    // Unlike plastic, a metal's highlight takes the metal's own color.
    const Material gold = {nullptr, nullptr, srgb({0.75164f, 0.60648f, 0.22648f}), srgb({0.628281f, 0.555802f, 0.366065f}), 51.2f * 4.0f, 0.4f}; // table value is for Phong

    // Wooden crate with a steel rim. The specular map is black over the wood and bright over the metal,
    // so only the rim catches highlights, and only the rim reflects the sky. White tints: the maps decide.
    const Material crate = {&scene.crateDiffuse, &scene.crateSpecular, glm::vec3(1.0f), glm::vec3(1.0f), 128.0f, 0.3f};

    // Chrome: almost no color of its own, a perfect mirror of the environment.
    const Material chrome = {nullptr, nullptr, glm::vec3(0.02f), glm::vec3(1.0f), 512.0f, 1.0f};

    // Glass: mostly see-through. 0.04 is real glass's head-on reflectivity; Fresnel raises it at the edges.
    const Material glass = {nullptr, nullptr, glm::vec3(0.02f), glm::vec3(1.0f), 512.0f, 0.04f, 0.9f, 1.52f};

    auto addMesh = [&](const char* name, const Mesh& mesh, const Material& material, const Transform& transform) {
        registry.emplace<MeshRenderer>(scene.create(name, transform), &mesh, material);
    };

    addMesh("Floor", scene.cube, rubber({0.6f, 0.6f, 0.6f}), {.position = {0.0f, -0.55f, 0.0f}, .scale = {10.0f, 0.1f, 10.0f}}); // top surface at y = -0.5
    addMesh("Orange sphere", scene.sphere, plastic({1.0f, 0.5f, 0.3f}), {.position = {0.0f, 0.0f, 0.0f}});
    addMesh("Blue sphere", scene.sphere, plastic({0.3f, 0.5f, 1.0f}, 512.0f), {.position = {2.0f, 0.0f, -1.0f}, .rotation = {0.0f, 30.0f, 0.0f}});
    addMesh("Green sphere", scene.sphere, rubber({0.4f, 0.9f, 0.4f}), {.position = {-1.5f, -0.25f, 1.0f}, .scale = glm::vec3(0.5f)});
    addMesh("Purple sphere", scene.sphere, plastic({0.7f, 0.05f, 0.87f}), {.position = {1.5f, 0.0f, 1.0f}, .rotation = {0.0f, 45.0f, 0.0f}});
    addMesh("Gold sphere", scene.sphere, gold, {.position = {-2.2f, 0.0f, -1.2f}, .rotation = {0.0f, 15.0f, 0.0f}});
    addMesh("Crate", scene.cube, crate, {.position = {0.0f, 0.0f, 2.4f}, .rotation = {0.0f, 25.0f, 0.0f}});
    addMesh("Crate", scene.cube, crate, {.position = {-3.2f, 0.0f, 0.6f}, .rotation = {0.0f, 10.0f, 0.0f}});
    addMesh("Chrome sphere", scene.sphere, chrome, {.position = {3.3f, 0.0f, 0.8f}});
    addMesh("Glass sphere", scene.sphere, glass, {.position = {-0.4f, 0.0f, -2.4f}});

    // "Shiba" by zixisun02, CC-BY-4.0 (see models/shiba/license.txt).
    const entt::entity dog = scene.create("Shiba", {.position = {3.6f, -0.5f, -1.6f}, .rotation = {0.0f, -30.0f, 0.0f}, .scale = glm::vec3(scene.shiba.fitScale(1.2f))});
    registry.emplace<ModelRenderer>(dog, &scene.shiba);

    DirLight sun;
    sun.ambient = glm::vec3(0.01f);
    sun.diffuse = srgb({0.45f, 0.42f, 0.38f}); // slightly warm, still dim enough for the point lights to stand out
    // Angled, so shadows are long enough to see.
    registry.emplace<DirLight>(scene.create("Sun", {.rotation = aimRotation({-0.6f, -1.0f, -0.4f})}), sun);

    // The point lights circle the scene because their parent spins, not because the renderer moves them.
    const entt::entity orbit = scene.create("Light orbit");
    registry.emplace<Rotator>(orbit, glm::vec3(0.0f, glm::degrees(0.5f), 0.0f));

    registry.emplace<PointLight>(scene.create("Red light", {.position = {2.0f, 1.0f, 1.5f}, .parent = orbit}), coloredLight({1.0f, 0.2f, 0.2f}));
    registry.emplace<PointLight>(scene.create("Green light", {.position = {-2.0f, 1.0f, 1.5f}, .parent = orbit}), coloredLight({0.2f, 1.0f, 0.2f}));
    registry.emplace<PointLight>(scene.create("Blue light", {.position = {-1.5f, 1.2f, -2.0f}, .parent = orbit}), coloredLight({0.2f, 0.3f, 1.0f}));
    registry.emplace<PointLight>(scene.create("White light", {.position = {2.0f, 1.5f, -2.0f}, .parent = orbit}), coloredLight({1.0f, 1.0f, 1.0f}));

    const entt::entity flashlight = scene.create("Flashlight"); // off by default
    registry.emplace<SpotLight>(flashlight);
    registry.emplace<FollowCamera>(flashlight);
}
