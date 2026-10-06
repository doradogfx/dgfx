#include "game/upgrades.h"

#include "game/game.h"
#include "game/weapons.h"
#include "game/xp.h"

const char* upgradeLabel(Upgrade upgrade) {
    switch (upgrade) {
    case Upgrade::Damage:
        return "Damage +25%";
    case Upgrade::FireRate:
        return "Fire rate +15%";
    case Upgrade::MoveSpeed:
        return "Move speed +10%";
    case Upgrade::PickupRadius:
        return "Pickup radius +1";
    case Upgrade::MaxHealth:
        return "Max health +20";
    default:
        return "";
    }
}

void applyUpgrade(Scene& scene, entt::entity player, Upgrade upgrade) {
    entt::registry& registry = scene.registry;

    // A missing component makes the upgrade do nothing. The editor can remove components.
    switch (upgrade) {
    case Upgrade::Damage:
        if (Weapon* weapon = registry.try_get<Weapon>(player))
            weapon->damage *= 1.25f;
        break;
    case Upgrade::FireRate:
        if (Weapon* weapon = registry.try_get<Weapon>(player))
            weapon->interval *= 0.85f;
        break;
    case Upgrade::MoveSpeed:
        if (Player* p = registry.try_get<Player>(player))
            p->speed *= 1.1f;
        break;
    case Upgrade::PickupRadius:
        if (Experience* experience = registry.try_get<Experience>(player))
            experience->pickupRadius += 1.0f;
        break;
    case Upgrade::MaxHealth:
        if (Health* health = registry.try_get<Health>(player)) {
            health->max += 20.0f;
            health->current += 20.0f;
        }
        break;
    default:
        break;
    }
}
