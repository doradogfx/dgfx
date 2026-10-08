#include "ui/panels.h"

#include "assets/assets.h"
#include "scene/component_registry.h"
#include "scene/scene.h"
#include "scene/scene_io.h"
#include "ui/ui.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
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

// Hierarchy options, kept between frames.
static char search[64] = "";
static bool showRuntime = true; // entities made while running (Transient): enemies, props, orbs

// Entities whose parent is `parent` (entt::null for roots), in creation order. Views iterate newest first.
// ponytail: scans every entity per call, keep a children list in the scene if trees get large.
static std::vector<entt::entity> children(const Scene& scene, entt::entity parent) {
    std::vector<entt::entity> result;

    for (auto [entity, transform] : scene.registry.view<const Transform>().each()) {
        if (transform.parent == parent && (showRuntime || !scene.registry.all_of<Transient>(entity)))
            result.push_back(entity);
    }

    std::sort(result.begin(), result.end());
    return result;
}

// A short word for what the entity is, from its engine components. Empty for an entity with none of them.
static const char* kindOf(const entt::registry& registry, entt::entity entity) {
    if (registry.all_of<Camera>(entity))
        return "Camera";
    if (registry.any_of<DirLight, PointLight, SpotLight>(entity))
        return "Light";
    if (registry.all_of<ModelRenderer>(entity))
        return "Model";
    if (registry.all_of<MeshRenderer>(entity))
        return "Mesh";
    return "";
}

// One table row: the entity's name (a tree node when it has children) and its kind. Runtime entities are dim,
// because a save does not keep them. Returns true when the node is open: then the caller draws the children
// and calls TreePop.
static bool entityRow(const Scene& scene, entt::entity entity, bool hasChildren) {
    const entt::registry& registry = scene.registry;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_FramePadding;

    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (entity == selected)
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool runtime = registry.all_of<Transient>(entity);

    if (runtime)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    // The entity id is the tree node's ID, so two entities with the same name ("Crate") stay distinct.
    const void* id = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(entt::to_integral(entity)));
    const bool open = ImGui::TreeNodeEx(id, flags, "%s", registry.get<Name>(entity).value.c_str());

    if (runtime)
        ImGui::PopStyleColor();

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        selected = entity;

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Add child"))
            pending = {Action::AddEntity, entity};
        if (ImGui::MenuItem("Add entity"))
            pending = {Action::AddEntity, registry.get<Transform>(entity).parent}; // next to this one

        ImGui::Separator();

        if (ImGui::MenuItem("Duplicate"))
            pending = {Action::Duplicate, entity};
        if (ImGui::MenuItem("Delete"))
            pending = {Action::Delete, entity};

        ImGui::EndPopup();
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", kindOf(registry, entity));

    return open && hasChildren;
}

static void entityNode(const Scene& scene, entt::entity entity) {
    const std::vector<entt::entity> kids = children(scene, entity);

    if (entityRow(scene, entity, !kids.empty())) {
        for (entt::entity kid : kids)
            entityNode(scene, kid);

        ImGui::TreePop();
    }
}

static bool containsIgnoringCase(std::string text, std::string part) {
    auto lower = [](std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    };

    lower(text);
    lower(part);
    return text.find(part) != std::string::npos;
}

// One table row: a label, then an X, Y and Z field. The colored button before each field sets that axis back
// to resetValue. Call it between BeginTable and EndTable of a 2-column table.
static void vec3Row(const char* label, glm::vec3& value, float resetValue, float speed, const char* format) {
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);

    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float button = ImGui::GetFrameHeight();
    const float field = std::max(1.0f, (ImGui::GetContentRegionAvail().x - 3.0f * button - 2.0f * spacing) / 3.0f);

    // The usual axis colors: X red, Y green, Z blue.
    const ImVec4 colors[3] = {{0.72f, 0.22f, 0.20f, 1.0f}, {0.30f, 0.58f, 0.20f, 1.0f}, {0.22f, 0.40f, 0.78f, 1.0f}};
    const char* axes[3] = {"X", "Y", "Z"};

    for (int i = 0; i < 3; i++) {
        if (i > 0)
            ImGui::SameLine(0.0f, spacing);

        ImGui::PushID(i);
        const ImVec4 c = colors[i];
        ImGui::PushStyleColor(ImGuiCol_Button, c);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c.x * 1.2f, c.y * 1.2f, c.z * 1.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(c.x * 0.8f, c.y * 0.8f, c.z * 0.8f, 1.0f));

        if (ImGui::Button(axes[i], ImVec2(button, button)))
            value[i] = resetValue;

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Click to set %s to %g", axes[i], resetValue);

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetNextItemWidth(field);
        ImGui::DragFloat("##value", &value[i], speed, 0.0f, 0.0f, format);
        ImGui::PopID();
    }

    ImGui::PopID();
}

// The selected entity: its name, transform and registered components.
void inspectorPanel(UiContext& context) {
    entt::registry& registry = context.scene.registry;

    if (!registry.valid(selected)) {
        ImGui::TextDisabled("Select an entity to inspect it");
        return;
    }

    // The name over the full width, with the entity id after it.
    char id[32];
    std::snprintf(id, sizeof(id), "#%u", static_cast<unsigned>(entt::to_integral(selected)));

    char name[128];
    std::snprintf(name, sizeof(name), "%s", registry.get<Name>(selected).value.c_str());

    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(id).x - style.ItemSpacing.x);

    if (ImGui::InputTextWithHint("##name", "Entity name", name, sizeof(name)))
        registry.get<Name>(selected).value = name;

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", id);

    ImGui::Spacing();

    // The transform is a section like the components, but it has no X: every entity has one.
    if (auto* transform = registry.try_get<Transform>(selected)) {
        ImGui::PushID("transform");

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (transform->parent != entt::null && registry.valid(transform->parent))
                ImGui::TextDisabled("Relative to %s", registry.get<Name>(transform->parent).value.c_str());

            if (ImGui::BeginTable("fields", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 4.5f);
                ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
                vec3Row("Position", transform->position, 0.0f, 0.05f, "%.2f");
                vec3Row("Rotation", transform->rotation, 0.0f, 1.0f, "%.1f°");
                vec3Row("Scale", transform->scale, 1.0f, 0.01f, "%.2f");
                ImGui::EndTable();
            }
        }

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
    const Scene& scene = context.scene;
    const ImGuiStyle& style = ImGui::GetStyle();

    // Toolbar: a search field, and a button that adds an entity at the root. With an entity selected, the new
    // one goes next to it (same parent), so a child can be added to a group without the context menu.
    const char* addLabel = "+ Add entity";
    const float addWidth = ImGui::CalcTextSize(addLabel).x + style.FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - addWidth - style.ItemSpacing.x);
    ImGui::InputTextWithHint("##search", "Search", search, sizeof(search));
    ImGui::SameLine();

    if (ImGui::Button(addLabel)) {
        const Transform* current = scene.registry.valid(selected) ? scene.registry.try_get<Transform>(selected) : nullptr;
        pending = {Action::AddEntity, current ? current->parent : entt::null};
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Adds an entity next to the selected one, or at the root");

    // The counts, and the runtime toggle at the right edge.
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%zu entities, %zu at runtime", static_cast<size_t>(scene.registry.view<const Name>().size()),
                        static_cast<size_t>(scene.registry.view<const Transient>().size()));
    ImGui::SameLine();
    const char* toggle = "Runtime";
    const float toggleWidth = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + ImGui::CalcTextSize(toggle).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - toggleWidth));
    ImGui::Checkbox(toggle, &showRuntime);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Show the entities made while playing (enemies, props, orbs). A save does not keep them.");

    ImGui::Spacing();

    bool noMatch = false;

    if (ImGui::BeginTable("entities", 2, ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("kind", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Camera").x);

        if (search[0] == '\0') {
            for (entt::entity root : children(scene, entt::null))
                entityNode(scene, root);
        } else {
            // While searching, the matches as a flat list, in creation order.
            std::vector<entt::entity> matches;

            for (auto [entity, name] : scene.registry.view<const Name>().each()) {
                if ((showRuntime || !scene.registry.all_of<Transient>(entity)) && containsIgnoringCase(name.value, search))
                    matches.push_back(entity);
            }

            std::sort(matches.begin(), matches.end());

            for (entt::entity entity : matches)
                entityRow(scene, entity, false);

            noMatch = matches.empty();
        }

        ImGui::EndTable();
    }

    if (noMatch)
        ImGui::TextDisabled("No entity matches \"%s\"", search);

    if (ImGui::BeginPopupContextWindow("hierarchy_menu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Add entity"))
            pending = {Action::AddEntity, entt::null};

        ImGui::EndPopup();
    }

    applyPending(context);
}
