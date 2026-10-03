#pragma once

#include "assets/assets.h"
#include "core/camera.h"
#include "core/input.h"
#include "core/window.h"
#include "renderer/renderer.h"
#include "scene/component_registry.h"
#include "scene/scene.h"

#include <string>

// The window, the engine subsystems and the main loop. A game derives from it, builds its scene in its own
// constructor and overrides the hooks.
class Application {
public:
    Application();
    virtual ~Application();

    // Scene files live in scenes/. loadScene empties the scene first.
    void loadScene(const std::string& file);
    void saveScene(const std::string& file) const;

    // Runs until the window closes.
    void run();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

protected:
    // Once per frame, after input is read and before the render.
    virtual void update(float dt) = 0;

    // Switches to a scene file, chosen from File > Open scene. A game overrides it to set up its own state;
    // the default just loads the scene. Called between frames, never in the middle of one.
    virtual void openScene(const std::string& file) { loadScene(file); }

    // Extra lines for the stats overlay.
    virtual void statsOverlay() {}

    // Declaration order matters: members are destroyed in reverse, so the window (and its GL context) goes
    // last, and the assets come before the scene whose components point into them.
    Window window;
    Assets assets;
    Scene scene;
    Renderer renderer;
    ComponentRegistry components;
    Input input; // before the UI starts, so ImGui chains to its scroll callback
    Camera camera;

private:
    std::string currentScene;
    std::string pendingScene; // requested by the UI, opened at the start of the next frame
};
