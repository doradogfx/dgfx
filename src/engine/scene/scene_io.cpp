#include "scene/scene_io.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

static Json textureToJson(const Texture& texture, const Assets& assets) {
    return Json{{"file", assets.name(texture)}, {"srgb", texture.isSrgb}};
}

Json materialToJson(const Material& material, const Assets& assets) {
    Json json = {
        {"diffuse", material.diffuse},
        {"specular", material.specular},
        {"shininess", material.shininess},
        {"reflectivity", material.reflectivity},
        {"refractivity", material.refractivity},
        {"ior", material.ior},
        {"doubleSided", material.doubleSided},
    };

    if (material.diffuseMap)
        json["diffuseMap"] = textureToJson(*material.diffuseMap, assets);
    if (material.specularMap)
        json["specularMap"] = textureToJson(*material.specularMap, assets);

    return json;
}

static const Texture* textureFromJson(const Json& json, const char* key, Assets& assets) {
    if (!json.contains(key))
        return nullptr;

    return &assets.texture(json[key].at("file").get<std::string>(), json[key].at("srgb").get<bool>());
}

// Every field is optional, so a material can be as short as {} (a plain gray).
template <typename T>
static T jsonOr(const Json& json, const char* key, T fallback) {
    return json.contains(key) ? json[key].get<T>() : fallback;
}

Material materialFromJson(const Json& json, Assets& assets) {
    Material material{};
    material.diffuseMap = textureFromJson(json, "diffuseMap", assets);
    material.specularMap = textureFromJson(json, "specularMap", assets);
    material.diffuse = jsonOr(json, "diffuse", glm::vec3(0.8f));
    material.specular = jsonOr(json, "specular", glm::vec3(0.3f));
    material.shininess = jsonOr(json, "shininess", 32.0f);
    material.reflectivity = jsonOr(json, "reflectivity", material.reflectivity);
    material.refractivity = jsonOr(json, "refractivity", material.refractivity);
    material.ior = jsonOr(json, "ior", material.ior);
    material.doubleSided = jsonOr(json, "doubleSided", material.doubleSided);
    return material;
}

// A float widened to a double prints with 17 digits (0.35 becomes 0.3499999940395355), so each number is
// re-read from its shortest float text first.
static void roundFloats(Json& json) {
    if (json.is_number_float())
        json = std::stod(std::format("{}", json.get<float>()));
    else if (json.is_array() || json.is_object())
        for (Json& child : json)
            roundFloats(child);
}

void saveScene(const Scene& scene, const Assets& assets, const ComponentRegistry& components, const std::string& path) {
    const entt::registry& registry = scene.registry;

    // Creation order, so loading gives the entities the same order (views iterate newest first).
    std::vector<entt::entity> entities;

    for (auto [entity, name] : registry.view<const Name>().each()) {
        if (!registry.all_of<Transient>(entity))
            entities.push_back(entity);
    }

    std::sort(entities.begin(), entities.end());

    std::unordered_map<entt::entity, int> index;

    for (size_t i = 0; i < entities.size(); i++)
        index[entities[i]] = static_cast<int>(i);

    Json root;
    root["entities"] = Json::array();

    if (scene.sky)
        root["sky"] = assets.name(*scene.sky);

    for (entt::entity entity : entities) {
        const Transform& transform = registry.get<Transform>(entity);

        Json transformJson = {{"position", transform.position}, {"rotation", transform.rotation}, {"scale", transform.scale}};

        // A parent that isn't saved (a transient entity) leaves the child as a root.
        if (const auto parent = index.find(transform.parent); parent != index.end())
            transformJson["parent"] = parent->second;

        Json object = components.save(registry, entity, assets);
        object["name"] = registry.get<Name>(entity).value;
        object["transform"] = transformJson;
        root["entities"].push_back(object);
    }

    std::ofstream file(path);

    if (!file)
        throw std::runtime_error("Can't write scene " + path);

    roundFloats(root);
    file << root.dump(2) << '\n';
}

void loadScene(Scene& scene, Assets& assets, const ComponentRegistry& components, const std::string& path) {
    std::ifstream file(path);

    if (!file)
        throw std::runtime_error("Can't open scene " + path);

    Json root;

    try {
        root = Json::parse(file);
    } catch (const Json::exception& e) {
        throw std::runtime_error("Bad scene file " + path + ": " + e.what());
    }

    try {
        scene.sky = root.contains("sky") ? &assets.cubemap(root["sky"].get<std::string>()) : nullptr;

        const Json& list = root.at("entities");

        // Two passes, so a parent can come anywhere in the file: create everyone, then fill them in.
        std::vector<entt::entity> entities;

        for (const Json& object : list)
            entities.push_back(scene.create(object.at("name").get<std::string>()));

        for (size_t i = 0; i < entities.size(); i++) {
            const Json& object = list[i];
            const Json& transformJson = object.at("transform");
            Transform& transform = scene.registry.get<Transform>(entities[i]);

            transform.position = transformJson.at("position").get<glm::vec3>();
            transform.rotation = transformJson.at("rotation").get<glm::vec3>();
            transform.scale = transformJson.at("scale").get<glm::vec3>();

            if (transformJson.contains("parent")) {
                const size_t parent = transformJson["parent"].get<size_t>();

                if (parent >= entities.size())
                    throw std::runtime_error("entity \"" + object.at("name").get<std::string>() + "\" has parent " + std::to_string(parent) + ", but there are only " + std::to_string(entities.size()) + " entities");

                transform.parent = entities[parent];
            }

            components.load(scene.registry, entities[i], object, assets, {"name", "transform"});
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Bad scene file " + path + ": " + e.what());
    }
}

entt::entity duplicateEntity(Scene& scene, Assets& assets, const ComponentRegistry& components, entt::entity source) {
    // Copied before create(): adding an entity can move the components in memory.
    const Transform transform = scene.registry.get<Transform>(source);
    const std::string name = scene.registry.get<Name>(source).value + " copy";
    const Json saved = components.save(scene.registry, source, assets);

    const entt::entity copy = scene.create(name, transform);
    components.load(scene.registry, copy, saved, assets, {});
    return copy;
}
