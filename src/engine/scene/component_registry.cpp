#include "scene/component_registry.h"

#include "renderer/light.h"
#include "scene/components.h"

#include <imgui.h>

void ComponentRegistry::inspect(entt::registry& registry, entt::entity entity) const {
    for (const Entry& entry : entries) {
        if (!entry.has(registry, entity))
            continue;

        // Each section gets its own ID scope, since labels like "Enabled" repeat.
        ImGui::PushID(entry.name.c_str());
        ImGui::SeparatorText(entry.name.c_str());
        entry.draw(registry, entity);
        ImGui::PopID();
    }
}

void registerEngineComponents(ComponentRegistry& components) {
    components.add<DirLight>("Directional light", [](DirLight& light) { lightUI(light); });
    components.add<PointLight>("Point light", [](PointLight& light) { lightUI(light); });
    components.add<SpotLight>("Spot light", [](SpotLight& light) { lightUI(light); });

    components.add<MeshRenderer>("Mesh renderer", [](MeshRenderer&) {
        ImGui::TextDisabled("Mesh with its own material");
    });

    components.add<ModelRenderer>("Model renderer", [](ModelRenderer& renderer) {
        ImGui::TextDisabled("%zu parts, materials from the file", renderer.model->parts.size());
    });
}
