#pragma once

#include "core/application.h"
#include "game/game.h"

#include <optional>

// The dgfx game on top of the engine. A scene with a Player entity is played (a Game exists, third-person
// camera); any other scene, like the demo, is just looked around with the fly camera. F1 toggles the fly
// camera while playing. Scenes are chosen from File > Open scene.
class GameApp : public Application {
public:
    GameApp();

protected:
    void update(float dt) override;
    void openScene(const std::string& file) override;
    void statsOverlay() override;

private:
    std::optional<Game> game;
    bool flyMode = false;
};
