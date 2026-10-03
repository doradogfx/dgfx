#include "scene/component_registry.h"

#include "assets/assets.h"
#include "renderer/light.h"
#include "scene/components.h"
#include "scene/scene_io.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

// Authored settings only, so a saved light round-trips and missing fields keep their defaults.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DirLight, enabled, ambient, diffuse, specular)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PointLight, enabled, ambient, diffuse, specular, constant, linear, quadratic)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpotLight, enabled, ambient, diffuse, specular, constant, linear, quadratic, innerAngle, outerAngle)

void ComponentRegistry::inspect(entt::registry& registry, entt::entity entity, Assets& assets) const {
    for (const Entry& entry : entries) {
        if (!entry.has(registry, entity))
            continue;

        // Each section gets its own ID scope, since labels like "Enabled" repeat.
        ImGui::PushID(entry.name.c_str());

        bool keep = true;

        if (ImGui::CollapsingHeader(entry.name.c_str(), &keep, ImGuiTreeNodeFlags_DefaultOpen))
            entry.draw(registry, entity);

        if (!keep)
            entry.remove(registry, entity);

        ImGui::PopID();
    }

    ImGui::Spacing();

    if (ImGui::Button("Add component"))
        ImGui::OpenPopup("add_component");

    if (ImGui::BeginPopup("add_component")) {
        for (const Entry& entry : entries) {
            if (entry.addable && !entry.has(registry, entity) && ImGui::MenuItem(entry.name.c_str()))
                entry.load(registry, entity, Json::object(), assets); // an empty object gives the defaults
        }

        ImGui::EndPopup();
    }
}

Json ComponentRegistry::save(const entt::registry& registry, entt::entity entity, const Assets& assets) const {
    Json object = Json::object();

    for (const Entry& entry : entries) {
        if (entry.has(registry, entity))
            object[entry.name] = entry.save(registry, entity, assets);
    }

    return object;
}

void ComponentRegistry::load(entt::registry& registry, entt::entity entity, const Json& object, Assets& assets, const std::vector<std::string>& ignore) const {
    for (const auto& [key, value] : object.items()) {
        if (std::find(ignore.begin(), ignore.end(), key) != ignore.end())
            continue;

        const auto it = std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) { return entry.name == key; });

        if (it == entries.end())
            std::fprintf(stderr, "Scene file: unknown component \"%s\", skipped\n", key.c_str());
        else
            it->load(registry, entity, value, assets);
    }
}

void registerEngineComponents(ComponentRegistry& components) {
    components.add<DirLight>("Directional light", [](DirLight& light) { lightUI(light); });
    components.add<PointLight>("Point light", [](PointLight& light) { lightUI(light); });
    components.add<SpotLight>("Spot light", [](SpotLight& light) { lightUI(light); });

    components.add<MeshRenderer>(
        "Mesh renderer",
        [](MeshRenderer&) { ImGui::TextDisabled("Mesh with its own material"); },
        [](const MeshRenderer& renderer, const Assets& assets) {
            return Json{{"mesh", assets.name(*renderer.mesh)}, {"material", materialToJson(renderer.material, assets)}};
        },
        [](const Json& json, Assets& assets) {
            const std::string mesh = json.contains("mesh") ? json["mesh"].get<std::string>() : "cube";
            return MeshRenderer{&assets.mesh(mesh), materialFromJson(json.value("material", Json::object()), assets)};
        });

    components.add<ModelRenderer>(
        "Model renderer",
        [](ModelRenderer& renderer) { ImGui::TextDisabled("%zu parts, materials from the file", renderer.model->parts.size()); },
        [](const ModelRenderer& renderer, const Assets& assets) {
            return Json{{"model", assets.name(*renderer.model)}};
        },
        [](const Json& json, Assets& assets) {
            return ModelRenderer{&assets.model(json.at("model").get<std::string>())};
        },
        false); // needs a model file, so the inspector can't add one from nothing
}
