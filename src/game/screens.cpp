#include "game/screens.h"

#include "game/upgrades.h"
#include "game/xp.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <cstdio>

// A window in the middle of the screen, without a title bar when `title` is null.
static bool beginCentered(const char* title) {
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, screen.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    return ImGui::Begin(title, nullptr, flags);
}

static bool bigButton(const char* label) {
    return ImGui::Button(label, ImVec2(ImGui::GetFontSize() * 16.0f, ImGui::GetFontSize() * 2.5f));
}

static bool enterPressed() {
    return ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
}

// Seconds as mm:ss.
static void formatTime(char* text, size_t size, float seconds) {
    const int total = static_cast<int>(seconds);
    std::snprintf(text, size, "%02d:%02d", total / 60, total % 60);
}

MenuAction mainMenu() {
    MenuAction action = MenuAction::None;

    if (beginCentered("dgfx")) {
        if (bigButton("Start") || enterPressed())
            action = MenuAction::Start;
        if (bigButton("Quit"))
            action = MenuAction::Quit;
    }

    ImGui::End();
    return action;
}

PauseAction pauseMenu() {
    PauseAction action = PauseAction::None;

    if (beginCentered("Paused")) {
        if (bigButton("Resume") || enterPressed())
            action = PauseAction::Resume;
        if (bigButton("Restart"))
            action = PauseAction::Restart;
        if (bigButton("Main menu"))
            action = PauseAction::Menu;
    }

    ImGui::End();
    return action;
}

GameOverAction endScreen(const RunState& state, const Scene& scene) {
    GameOverAction action = GameOverAction::None;

    if (beginCentered(state.won ? "You win!" : "Game over")) {
        char time[16];
        formatTime(time, sizeof(time), state.elapsed);
        ImGui::Text("Survived %s", time);

        if (const Experience* experience = scene.registry.valid(state.player) ? scene.registry.try_get<Experience>(state.player) : nullptr)
            ImGui::Text("Level %d", experience->level);

        ImGui::Text("Kills %d", state.kills);
        ImGui::Spacing();

        if (bigButton("Restart") || enterPressed())
            action = GameOverAction::Restart;
        if (bigButton("Main menu"))
            action = GameOverAction::Menu;
    }

    ImGui::End();
    return action;
}

int levelUpChoice(const RunState& state) {
    int chosen = -1;

    if (beginCentered("Level up!")) {
        ImGui::Text("Choose an upgrade");
        ImGui::Spacing();

        const ImGuiKey keys[] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3};

        for (int i = 0; i < static_cast<int>(state.choices.size()); i++) {
            char label[64];
            std::snprintf(label, sizeof(label), "%d. %s", i + 1, upgradeLabel(state.choices[i]));

            if (bigButton(label) || ImGui::IsKeyPressed(keys[i], false))
                chosen = i;
        }
    }

    ImGui::End();
    return chosen;
}

// A bar with its text in the middle. ImGui puts the text of a progress bar at the end of the fill, so the
// text moves while the bar fills. This draws the bar with no text and the text on top of it.
static void bar(float fraction, float width, const char* text, ImVec4 color) {
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(fraction, ImVec2(width, 0.0f), "");
    ImGui::PopStyleColor();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 position((min.x + max.x - size.x) * 0.5f, (min.y + max.y - size.y) * 0.5f);
    ImGui::GetWindowDrawList()->AddText(position, ImGui::GetColorU32(ImGuiCol_Text), text);
}

void hud(const RunState& state, const Scene& scene) {
    if (!scene.registry.valid(state.player)) // the editor can delete the player
        return;

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, ImGui::GetFrameHeight() * 1.5f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);

    // The HUD only shows information. It must not take mouse clicks or keys from the state.
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("HUD", nullptr, flags)) {
        const float width = ImGui::GetFontSize() * 20.0f;
        char text[64];

        if (const Health* health = scene.registry.try_get<Health>(state.player)) {
            std::snprintf(text, sizeof(text), "%.0f / %.0f", health->current, health->max);
            bar(health->max > 0.0f ? health->current / health->max : 0.0f, width, text, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
        }

        if (const Experience* experience = scene.registry.try_get<Experience>(state.player)) {
            std::snprintf(text, sizeof(text), "Level %d", experience->level);
            bar(experience->xp / xpToNext(*experience), width, text, ImVec4(0.3f, 0.7f, 1.0f, 1.0f)); // the orb color
        }

        char time[16];
        formatTime(time, sizeof(time), state.elapsed);
        ImGui::Text("%s   |   %d kills", time, state.kills);

        if (const Health* health = scene.registry.valid(state.boss) ? scene.registry.try_get<Health>(state.boss) : nullptr) {
            std::snprintf(text, sizeof(text), "Boss %.0f / %.0f", health->current, health->max);
            bar(health->current / health->max, width, text, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        }
    }

    ImGui::End();
}

void Game::restart(Screen next) {
    openScene(sceneName());
    screen = next;
}

void Game::gameUI() {
    const bool playing = state.player != entt::null;
    const bool choosing = playing && screen == Screen::Playing && state.pendingLevelUps > 0;
    const bool wantsCursor = playing && (screen != Screen::Playing || choosing);

    // A free cursor on the menus and the level-up choice, so the player can click. Every frame, because a click
    // outside the window captures the cursor again. Captured once when play continues, so Tab still frees it
    // for the debug panels.
    if (wantsCursor)
        input.setCaptured(false);
    else if (cursorFree)
        input.setCaptured(true);

    cursorFree = wantsCursor;

    if (!playing)
        return;

    // A change of screen can load the scene again, which starts a new run. Return after it.
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
        hud(state, scene);

        if (choosing) {
            if (const int chosen = levelUpChoice(state); chosen >= 0) {
                audio.play("ui_select.wav");
                choose(chosen);
            }
        }
        return;

    case Screen::Paused: {
        hud(state, scene);
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
        const GameOverAction action = endScreen(state, scene);

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
