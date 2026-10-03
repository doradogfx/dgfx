#pragma once

#include <entt/entt.hpp>

#include <functional>
#include <string>
#include <vector>

// The component types the editor knows about, each with a function that draws its fields. The inspector
// loops over these instead of naming every component, so the engine UI never depends on game components.
class ComponentRegistry {
public:
    // `inspect` takes the component by reference and draws its ImGui widgets, e.g. [](Health& h) { ... }.
    template <typename T, typename Inspect>
    void add(const std::string& name, Inspect inspect) {
        entries.push_back({
            name,
            [](entt::registry& registry, entt::entity entity) { return registry.all_of<T>(entity); },
            [inspect](entt::registry& registry, entt::entity entity) { inspect(registry.get<T>(entity)); },
        });
    }

    // Draws a titled section for each registered component the entity has, in registration order.
    void inspect(entt::registry& registry, entt::entity entity) const;

private:
    struct Entry {
        std::string name;
        std::function<bool(entt::registry&, entt::entity)> has;
        std::function<void(entt::registry&, entt::entity)> draw;
    };

    std::vector<Entry> entries;
};

// Registers the engine's own components: rotators, lights, mesh and model renderers.
void registerEngineComponents(ComponentRegistry& components);
