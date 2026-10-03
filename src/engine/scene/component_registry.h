#pragma once

#include "scene/json.h"

#include <entt/entt.hpp>

#include <functional>
#include <string>
#include <type_traits>
#include <vector>

class Assets;

// The component types the engine knows about, each with a function that draws its fields in the inspector and
// functions that save it to and load it from a scene file. The inspector and the scene loader loop over these
// instead of naming every component, so the engine never depends on game components.
// The name is the component's key in scene files.
// ponytail: renaming a section breaks old scene files, give entries a separate stable key if that starts to hurt.
class ComponentRegistry {
public:
    // A component with plain data: `inspect` draws its ImGui widgets, e.g. [](Health& h) { ... }, and it is saved
    // through its Json conversion (to_json/from_json, usually from NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT).
    template <typename T, typename Inspect>
    void add(const std::string& name, Inspect inspect) {
        add<T>(
            name, inspect,
            [](const T& component, const Assets&) { return Json(component); },
            [](const Json& json, Assets&) { return json.get<T>(); });
    }

    // A component that needs the assets to be saved or loaded (it holds pointers into them).
    // save: (const T&, const Assets&) -> Json.  load: (const Json&, Assets&) -> T.
    // `addable` is false when load can't build one from an empty object, so the inspector doesn't offer it.
    template <typename T, typename Inspect, typename Save, typename Load>
    void add(const std::string& name, Inspect inspect, Save save, Load load, bool addable = true) {
        // EnTT stores empty types (tags) without data, so there's nothing to get(): a default one stands in.
        entries.push_back({
            name,
            addable,
            [](const entt::registry& registry, entt::entity entity) { return registry.all_of<T>(entity); },
            [inspect](entt::registry& registry, entt::entity entity) {
                if constexpr (std::is_empty_v<T>) {
                    T tag;
                    inspect(tag);
                } else {
                    inspect(registry.get<T>(entity));
                }
            },
            [save](const entt::registry& registry, entt::entity entity, const Assets& assets) {
                if constexpr (std::is_empty_v<T>)
                    return save(T{}, assets);
                else
                    return save(registry.get<T>(entity), assets);
            },
            [load](entt::registry& registry, entt::entity entity, const Json& json, Assets& assets) {
                if constexpr (std::is_empty_v<T>)
                    registry.emplace_or_replace<T>(entity);
                else
                    registry.emplace_or_replace<T>(entity, load(json, assets));
            },
            [](entt::registry& registry, entt::entity entity) { registry.remove<T>(entity); },
        });
    }

    // The inspector body: a collapsing section per registered component the entity has (its X removes the
    // component), then an "Add component" button listing the ones it lacks.
    void inspect(entt::registry& registry, entt::entity entity, Assets& assets) const;

    // The entity's registered components as a JSON object {section name: data}.
    Json save(const entt::registry& registry, entt::entity entity, const Assets& assets) const;

    // Adds the components found in `object` to the entity. Keys that aren't registered are skipped with a warning,
    // except the ones in `ignore`.
    void load(entt::registry& registry, entt::entity entity, const Json& object, Assets& assets, const std::vector<std::string>& ignore) const;

private:
    struct Entry {
        std::string name;
        bool addable;
        std::function<bool(const entt::registry&, entt::entity)> has;
        std::function<void(entt::registry&, entt::entity)> draw;
        std::function<Json(const entt::registry&, entt::entity, const Assets&)> save;
        std::function<void(entt::registry&, entt::entity, const Json&, Assets&)> load;
        std::function<void(entt::registry&, entt::entity)> remove;
    };

    std::vector<Entry> entries;
};

// Registers the engine's own components: lights, mesh and model renderers.
void registerEngineComponents(ComponentRegistry& components);
