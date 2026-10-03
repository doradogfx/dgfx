#pragma once

#include "assets/assets.h"
#include "scene/component_registry.h"
#include "scene/scene.h"

#include <string>

// Scene files are JSON: {"sky": "skybox/", "entities": [...]}. An entity's id is its index in the array, so a
// transform's "parent" is just an index. Throws with the path in the message if the file can't be read or parsed.
void saveScene(const Scene& scene, const Assets& assets, const ComponentRegistry& components, const std::string& path);

// Fills a scene from a file. The scene should be empty.
void loadScene(Scene& scene, Assets& assets, const ComponentRegistry& components, const std::string& path);

// Materials hold pointers to textures, so they need the assets to be saved or loaded.
Json materialToJson(const Material& material, const Assets& assets);
Material materialFromJson(const Json& json, Assets& assets);

// A copy of one entity (not its children) with the same parent, made by saving and loading its components.
entt::entity duplicateEntity(Scene& scene, Assets& assets, const ComponentRegistry& components, entt::entity source);
