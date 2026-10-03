#pragma once

#include <functional>

struct GLFWwindow;
struct Scene;
class ComponentRegistry;
class Renderer;

// ImGui setup and per-frame boilerplate.
void initUI(GLFWwindow* window);
void beginUI();
void endUI(); // draws the UI on top of whatever is in the window
void shutdownUI();

// The debug UI: menu bar, hierarchy, inspector, renderer and display settings, stats overlay.
// `statsExtra` draws extra lines inside the stats overlay (the game's own numbers).
void debugUI(GLFWwindow* window, Scene& scene, Renderer& renderer, const ComponentRegistry& components, const std::function<void()>& statsExtra);

// Applies a window mode/resolution change picked in the panel. Call between frames, after glfwSwapBuffers.
void applyDisplayChanges(GLFWwindow* window);
