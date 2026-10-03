#include "ui/panels.h"

#include "assets/assets.h"
#include "scene/component_registry.h"
#include "scene/scene.h"
#include "scene/scene_io.h"
#include "ui/ui.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

static entt::entity selected = entt::null;

// What a hierarchy context menu asked for. Applied after the panel is drawn: the tree walks the registry, so
// creating or destroying entities in the middle of it would break the walk.
enum class Action { None, AddEntity, Duplicate, Delete };

struct Pending {
    Action action = Action::None;
    entt::entity target = entt::null; // the entity right-clicked, or null for empty space
};

static Pending pending;

// Entities whose parent is `parent` (entt::null for roots), in creation order. Views iterate newest first.
// ponytail: scans every entity per call, keep a children list in the scene if trees get large.
static std::vector<entt::entity> children(const Scene& scene, entt::entity parent) {
    std::vector<entt::entity> result;

    for (auto [entity, transform] : scene.registry.view<const Transform>().each()) {
        if (transform.parent == parent)
            result.push_back(entity);
    }

    std::sort(result.begin(), result.end());
    return result;
}

static void entityNode(const Scene& scene, entt::entity entity) {
    const std::vector<entt::entity> kids = children(scene, entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (kids.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (entity == selected)
        flags |= ImGuiTreeNodeFlags_Selected;

    // The entity id is the tree node's ID, so two entities with the same name ("Crate") stay distinct.
    const void* id = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(entt::to_integral(entity)));
    const bool open = ImGui::TreeNodeEx(id, flags, "%s", scene.registry.get<Name>(entity).value.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        selected = entity;

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Add child"))
            pending = {Action::AddEntity, entity};
        if (ImGui::MenuItem("Duplicate"))
            pending = {Action::Duplicate, entity};
        if (ImGui::MenuItem("Delete"))
            pending = {Action::Delete, entity};

        ImGui::EndPopup();
    }

    if (open) {
        for (entt::entity kid : kids)
            entityNode(scene, kid);

        ImGui::TreePop();
    }
}

// The selected entity: its name, transform and registered components.
void inspectorPanel(UiContext& context) {
    entt::registry& registry = context.scene.registry;

    if (!registry.valid(selected)) {
        ImGui::TextDisabled("Select an entity to inspect it");
        return;
    }

    char name[128];
    std::snprintf(name, sizeof(name), "%s", registry.get<Name>(selected).value.c_str());

    if (ImGui::InputText("Name", name, sizeof(name)))
        registry.get<Name>(selected).value = name;

    if (auto* transform = registry.try_get<Transform>(selected)) {
        ImGui::PushID("transform");
        ImGui::DragFloat3("Position", glm::value_ptr(transform->position), 0.05f);
        ImGui::DragFloat3("Rotation", glm::value_ptr(transform->rotation), 1.0f, 0.0f, 0.0f, "%.1f deg");
        ImGui::DragFloat3("Scale", glm::value_ptr(transform->scale), 0.01f);
        ImGui::PopID();
    }

    context.components.inspect(registry, selected, context.assets);
}

// Applies what a hierarchy context menu asked for, now that nothing is walking the registry.
static void applyPending(UiContext& context) {
    const Pending request = pending;
    pending = {};

    switch (request.action) {
    case Action::None:
        break;
    case Action::AddEntity:
        selected = context.scene.create("Entity", {.parent = request.target});
        break;
    case Action::Duplicate:
        selected = duplicateEntity(context.scene, context.assets, context.components, request.target);
        break;
    case Action::Delete:
        context.scene.destroy(request.target); // a destroyed selection fails registry.valid() and clears itself
        break;
    }
}

void hierarchyPanel(UiContext& context) {
    for (entt::entity root : children(context.scene, entt::null))
        entityNode(context.scene, root);

    if (ImGui::BeginPopupContextWindow("hierarchy_menu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Add entity"))
            pending = {Action::AddEntity, entt::null};

        ImGui::EndPopup();
    }

    applyPending(context);
}
