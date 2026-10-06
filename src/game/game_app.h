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
    void gameUI() override;

private:
    // The screens of a scene with a player. The level-up choice is not a screen: it shows over Playing.
    // ponytail: an enum and a switch. Use a class for each state when one screen gets its own logic.
    enum class Screen { Menu, Playing, Paused, GameOver };

    // Loads the open scene again, which gives a fresh run.
    void restart(Screen next);

    std::optional<Game> game;
    Screen screen = Screen::Menu;
    bool flyMode = false;
    bool cursorFree = false; // the game UI freed the cursor last frame
};
