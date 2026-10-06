#pragma once

#include "core/camera.h"

#include <glm/glm.hpp>

#include <array>

struct GLFWwindow;

// Mouse and keyboard state for the frame. Two modes: captured (cursor hidden and locked to the window, so the
// mouse can turn forever without hitting the screen edge) and UI mode (normal cursor, for the ImGui panels).
// Tab toggles them.
class Input {
public:
    // Installs the scroll callback. Create before initUI(): ImGui chains to callbacks that already exist.
    explicit Input(GLFWwindow* window);

    void update();

    // Both are false while ImGui has the keyboard (typing in a text field).
    bool down(int key) const;
    bool pressed(int key) const; // went down this frame

    glm::vec2 look{0.0f}; // mouse movement in pixels this frame, 0 when not captured
    float scroll = 0.0f;  // wheel notches this frame, 0 over ImGui

    // Captured: cursor hidden and locked. Free: normal cursor, for the UI.
    void setCaptured(bool value);

    // The window's user pointer points at this object, so it must not be copied or moved.
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

private:

    GLFWwindow* window;
    bool captured = false;
    bool keyboardFree = true;
    std::array<bool, 512> keys{}; // GLFW_KEY_LAST is 348
    std::array<bool, 512> lastKeys{};
    double lastX = 0.0;
    double lastY = 0.0;
    double wheel = 0.0; // accumulated by the scroll callback, consumed by update()
};

// Free-flying debug camera: mouse look, WASD, Space/Ctrl up/down, Shift faster, scroll zoom.
void flyCamera(Camera& camera, const Input& input, float dt);
