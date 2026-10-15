#pragma once

#include "assets/assets.h"
#include "audio/audio.h"
#include "core/input.h"
#include "core/window.h"
#include "renderer/renderer.h"
#include "scene/component_registry.h"
#include "scene/scene.h"
#include "ui/ui.h"

#include <string>

// The engine: the window, the subsystems and the frame loop. A game derives from it, sets itself up in its
// constructor (registers its components, opens its first scene) and overrides the hooks.
//
// Each frame, run() does:
//   1. input.update(), then update(dt) or the fly camera, then scene.update(dt)
//   2. the debug UI (editor build), then gameUI()
//   3. renderer.render(), then the swap
//   4. between frames: the display changes and the scene picked in the debug UI
class Application {
public:
    Application();
    virtual ~Application() = default;

    // Runs the frame loop until the window closes.
    void run();

    // Scene files live in scenes/. openScene replaces the scene with the file's, then calls sceneOpened().
    void openScene(const std::string& file);
    void saveScene() const; // into the file that is open. Throws if it can't write it.
    const std::string& sceneName() const { return currentScene; } // empty if none

    // The hooks. update() is not called while the fly camera is on.
    virtual void update(float dt) = 0;
    virtual void sceneOpened() {}  // set up the game's state for the new scene
    virtual void gameUI() {}       // game windows: menus, HUD, choices
    virtual void statsOverlay() {} // extra lines for the stats overlay

    // The debug fly camera: F1 toggles it in the editor build. A game turns it on for a scene with nothing to play.
    bool flyMode = false;

    // The parts of the engine. Start-up makes them in this order, and shut-down destroys them in the reverse
    // order: the window (and its GL context) goes last, and the assets outlive the scene that points into them.
    Window window;
    Audio audio;
    Assets assets;
    Scene scene;
    Renderer renderer;
    ComponentRegistry components;
    Input input;
    Ui ui; // after the input, so ImGui chains to its scroll callback

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    std::string currentScene;
};
