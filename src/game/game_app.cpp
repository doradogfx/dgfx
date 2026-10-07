#include "game/game_app.h"

#include "game/demo.h"
#include "game/screens.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

GameApp::GameApp() {
    registerGameComponents(components);
    openScene("level.json");
    screen = Screen::Menu;
}

void GameApp::openScene(const std::string& file) {
    loadScene(file);
    game.reset();
    flyMode = false;
    screen = Screen::Playing;

    // The starting view for scenes without a player to follow.
    camera = Camera();
    camera.position = glm::vec3(0.0f, 2.0f, 6.0f);
    camera.pitch = -15.0f;

    if (!scene.registry.view<Player>().empty())
        game.emplace(scene, assets);
}

void GameApp::restart(Screen next) {
    openScene(sceneName());
    screen = next;
}

void GameApp::update(float dt) {
    if (input.pressed(GLFW_KEY_F1))
        flyMode = !flyMode;

    if (game) {
        if (input.pressed(GLFW_KEY_ESCAPE) && screen == Screen::Playing)
            screen = Screen::Paused;
        else if (input.pressed(GLFW_KEY_ESCAPE) && screen == Screen::Paused)
            screen = Screen::Playing;

        if ((game->dead || game->won) && screen == Screen::Playing) {
            screen = Screen::GameOver;
            audio.play(game->won ? "win.wav" : "game_over.wav");
        }
    }

    if (flyMode || !game)
        flyCamera(camera, input, dt);
    else if (screen == Screen::Playing)
        game->update(scene, camera, input, audio, dt);

    if (!game)
        updateDemo(scene, camera, dt);
}

void GameApp::statsOverlay() {
    ImGui::Text("%zu enemies", static_cast<size_t>(scene.registry.view<const Enemy>().size()));
    ImGui::TextDisabled("F1: fly camera");
}

void GameApp::gameUI() {
    const bool choosing = game && screen == Screen::Playing && game->pendingLevelUps > 0;
    const bool wantsCursor = game && (screen != Screen::Playing || choosing);

    // A free cursor on the menus and the level-up choice, so the player can click. Every frame, because a click
    // outside the window captures the cursor again. Captured once when play continues, so Tab still frees it
    // for the debug panels.
    if (wantsCursor)
        input.setCaptured(false);
    else if (cursorFree)
        input.setCaptured(true);

    cursorFree = wantsCursor;

    if (!game)
        return;

    // A change of screen can load the scene again, which replaces the game. Return after it.
    switch (screen) {
    case Screen::Menu: {
        const MenuAction action = mainMenu();

        if (action != MenuAction::None)
            audio.play("ui_select.wav");

        if (action == MenuAction::Start)
            screen = Screen::Playing;
        else if (action == MenuAction::Quit)
            glfwSetWindowShouldClose(window.handle(), GLFW_TRUE);
        return;
    }

    case Screen::Playing:
        hud(*game, scene);

        if (choosing) {
            if (const int chosen = levelUpChoice(*game); chosen >= 0) {
                audio.play("ui_select.wav");
                game->choose(scene, chosen);
            }
        }
        return;

    case Screen::Paused: {
        hud(*game, scene);
        const PauseAction action = pauseMenu();

        if (action != PauseAction::None)
            audio.play("ui_select.wav");

        if (action == PauseAction::Resume)
            screen = Screen::Playing;
        else if (action == PauseAction::Restart)
            restart(Screen::Playing);
        else if (action == PauseAction::Menu)
            restart(Screen::Menu);
        return;
    }

    case Screen::GameOver: {
        const GameOverAction action = endScreen(*game, scene);

        if (action != GameOverAction::None)
            audio.play("ui_select.wav");

        if (action == GameOverAction::Restart)
            restart(Screen::Playing);
        else if (action == GameOverAction::Menu)
            restart(Screen::Menu);
        return;
    }
    }
}
