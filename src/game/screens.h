#pragma once

#include "game/game.h"
#include "scene/scene.h"

// The game windows: menu, pause, game over, HUD and the level-up choice. Each function draws its window and
// returns what the player chose. GameApp changes the screen. Every button also has a key.

enum class MenuAction { None, Start, Quit };
enum class PauseAction { None, Resume, Restart, Menu };
enum class GameOverAction { None, Restart, Menu };

MenuAction mainMenu();                                          // Enter = Start
PauseAction pauseMenu();                                        // Enter = Resume
GameOverAction gameOverScreen(const Game& game, const Scene& scene); // Enter = Restart

// Returns the index of the chosen upgrade, or -1. Keys 1, 2 and 3 choose too.
int levelUpChoice(const Game& game);

// Health, XP and level, the time and the kills, at the top of the screen.
void hud(const Game& game, const Scene& scene);
