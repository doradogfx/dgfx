#pragma once

#include <functional>
#include <string>

struct GLFWwindow;
struct Scene;
class Assets;
class ComponentRegistry;
class Renderer;

// ImGui setup and per-frame boilerplate.
void initUI(GLFWwindow* window);
void beginUI();
void endUI(); // draws the UI on top of whatever is in the window
void shutdownUI();

// What the debug UI works on, built by the application each frame.
struct UiContext {
    Scene& scene;
    Assets& assets;
    Renderer& renderer;
    ComponentRegistry& components;
    std::string sceneName;                // the loaded scene file, empty if none
    std::function<bool()> save;           // File > Save scene; false if it failed
    std::function<void()> statsExtra;     // extra lines inside the stats overlay (the game's own numbers)
};

// The debug UI: menu bar, hierarchy, inspector, renderer and display settings, stats overlay.
void debugUI(GLFWwindow* window, UiContext& context);

// Applies a window mode/resolution change picked in the panel. Call between frames, after glfwSwapBuffers.
void applyDisplayChanges(GLFWwindow* window);
