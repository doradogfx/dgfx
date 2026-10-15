#pragma once

struct GLFWwindow;
struct Scene;
class Assets;
class ComponentRegistry;
class Renderer;

// The contents of the debug UI's panels. Each draws inside a window that debugUI has already begun.
// The entity tree, and its right-click menus.
void hierarchyPanel(Scene& scene, Assets& assets, const ComponentRegistry& components);
// The selected entity's name, transform and components.
void inspectorPanel(Scene& scene, Assets& assets, const ComponentRegistry& components);
void rendererPanel(Renderer& renderer);
void displayPanel(GLFWwindow* window);

// The UI scale (1.0 = default size), for sizing things in pixels.
float uiScale();

// Picks the starting scale from the monitor's size and applies it. Call once the ImGui context exists.
void initUiScale();
