#include "game/screens.h"

#include "game/upgrades.h"
#include "game/xp.h"

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

GameOverAction endScreen(const Game& game, const Scene& scene) {
    GameOverAction action = GameOverAction::None;

    if (beginCentered(game.won ? "You win!" : "Game over")) {
        char time[16];
        formatTime(time, sizeof(time), game.elapsed);
        ImGui::Text("Survived %s", time);

        if (const Experience* experience = scene.registry.valid(game.player) ? scene.registry.try_get<Experience>(game.player) : nullptr)
            ImGui::Text("Level %d", experience->level);

        ImGui::Text("Kills %d", game.kills);
        ImGui::Spacing();

        if (bigButton("Restart") || enterPressed())
            action = GameOverAction::Restart;
        if (bigButton("Main menu"))
            action = GameOverAction::Menu;
    }

    ImGui::End();
    return action;
}

int levelUpChoice(const Game& game) {
    int chosen = -1;

    if (beginCentered("Level up!")) {
        ImGui::Text("Choose an upgrade");
        ImGui::Spacing();

        const ImGuiKey keys[] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3};

        for (int i = 0; i < static_cast<int>(game.choices.size()); i++) {
            char label[64];
            std::snprintf(label, sizeof(label), "%d. %s", i + 1, upgradeLabel(game.choices[i]));

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

void hud(const Game& game, const Scene& scene) {
    if (!scene.registry.valid(game.player)) // the editor can delete the player
        return;

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, ImGui::GetFrameHeight() * 1.5f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);

    // The HUD only shows information. It must not take mouse clicks or keys from the game.
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("HUD", nullptr, flags)) {
        const float width = ImGui::GetFontSize() * 20.0f;
        char text[64];

        if (const Health* health = scene.registry.try_get<Health>(game.player)) {
            std::snprintf(text, sizeof(text), "%.0f / %.0f", health->current, health->max);
            bar(health->max > 0.0f ? health->current / health->max : 0.0f, width, text, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
        }

        if (const Experience* experience = scene.registry.try_get<Experience>(game.player)) {
            std::snprintf(text, sizeof(text), "Level %d", experience->level);
            bar(experience->xp / xpToNext(*experience), width, text, ImVec4(0.3f, 0.7f, 1.0f, 1.0f)); // the orb color
        }

        char time[16];
        formatTime(time, sizeof(time), game.elapsed);
        ImGui::Text("%s   |   %d kills", time, game.kills);

        if (const Health* health = scene.registry.valid(game.boss) ? scene.registry.try_get<Health>(game.boss) : nullptr) {
            std::snprintf(text, sizeof(text), "Boss %.0f / %.0f", health->current, health->max);
            bar(health->current / health->max, width, text, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        }
    }

    ImGui::End();
}
