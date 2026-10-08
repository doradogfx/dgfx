#pragma once

#include "scene/scene.h"

#include <entt/entt.hpp>

// A choice that the player gets at each level-up. Each one changes a value on the player entity.
enum class Upgrade { Damage, FireRate, MoveSpeed, PickupRadius, MaxHealth, OrbitBlade, Count };

// The text on the choice button, for example "Damage +25%".
const char* upgradeLabel(Upgrade upgrade);

void applyUpgrade(Scene& scene, entt::entity player, Upgrade upgrade);
