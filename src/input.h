#pragma once

#include "camera.h"

struct GLFWwindow;

// Fly-camera controls. Two modes: camera mode (cursor hidden and locked to the window, so the mouse can turn
// forever without hitting the screen edge) and UI mode (normal cursor, for the ImGui panel). Tab toggles them.
class CameraInput {
public:
    // Installs the scroll callback. Create before initUI(): ImGui chains to callbacks that already exist.
    explicit CameraInput(GLFWwindow* window);

    void update(Camera& camera, float dt);

    // The window's user pointer points at this object, so it must not be copied or moved.
    CameraInput(const CameraInput&) = delete;
    CameraInput& operator=(const CameraInput&) = delete;

private:
    void setCaptured(bool value);

    GLFWwindow* window;
    bool captured = false;
    bool tabWasDown = false;
    double lastX = 0.0; // mouse look works on how far the cursor moved since last frame
    double lastY = 0.0;
    double scroll = 0.0; // accumulated by the scroll callback, consumed by update()
};
