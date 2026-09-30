#pragma once

struct GLFWwindow;
struct Scene;
class Renderer;

// ImGui setup and per-frame boilerplate.
void initUI(GLFWwindow* window);
void beginUI();
void endUI(); // draws the UI on top of whatever is in the window
void shutdownUI();

// The dgfx debug window: stats, lights, render and post settings.
void debugPanel(Scene& scene, Renderer& renderer);
