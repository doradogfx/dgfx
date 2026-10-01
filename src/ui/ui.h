#pragma once

struct GLFWwindow;
struct Scene;
class Renderer;

// ImGui setup and per-frame boilerplate.
void initUI(GLFWwindow* window);
void beginUI();
void endUI(); // draws the UI on top of whatever is in the window
void shutdownUI();

// The debug UI: menu bar, hierarchy, inspector, renderer and display settings, stats overlay.
void debugUI(GLFWwindow* window, Scene& scene, Renderer& renderer);

// Applies a window mode/resolution change picked in the panel. Call between frames, after glfwSwapBuffers.
void applyDisplayChanges(GLFWwindow* window);
