#pragma once

#include "core/application.h"
#include "game/game.h"

#include <optional>

// The dgfx game on top of the engine. F2 switches between the game level (a Game exists) and the engine
// demo (none, so always the fly camera). F1 toggles the fly camera in the game.
class GameApp : public Application {
public:
    GameApp();

protected:
    void update(float dt) override;
    void statsOverlay() override;

private:
    std::optional<Game> game;
    bool flyMode = false;
};
