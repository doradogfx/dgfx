#pragma once

#include <string>

struct GLFWwindow;
class Application;

// Dear ImGui for the window. Make it after Input: ImGui chains to the input callbacks that already exist.
class Ui {
public:
    explicit Ui(GLFWwindow* window);
    ~Ui();

    void beginFrame();
    void endFrame(); // draws the UI on top of whatever is in the window

    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;
};

// The debug UI: menu bar, hierarchy, inspector, renderer and display settings, stats overlay.
// Returns the scene file picked in File > Open scene, or an empty string. The application opens it between frames.
std::string debugUI(Application& app);

// Applies a window mode/resolution change picked in the panel. Call between frames, after glfwSwapBuffers.
void applyDisplayChanges(GLFWwindow* window);
