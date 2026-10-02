#include "ui/ui.h"

#include "game/game.h"
#include "renderer/renderer.h"
#include "scene/scene.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

static bool vsync = true;
static float uiScale = 1.0f;

// Scales text and every padding/spacing/size. ScaleAllSizes multiplies the current values,
// so it's applied to a fresh default style each time instead of compounding.
static void applyUiScale(float scale) {
    ImGuiStyle style;
    ImGui::StyleColorsDark(&style);
    style.ScaleAllSizes(scale);
    style.FontScaleMain = scale;
    ImGui::GetStyle() = style;
}

struct Resolution {
    int width;
    int height;
};

static std::vector<Resolution> supportedResolutions(GLFWmonitor* monitor) {
    int count;
    const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
    std::vector<Resolution> result;

    for (int i = count - 1; i >= 0; i--) {
        const Resolution r = {modes[i].width, modes[i].height};
        const bool seen = std::any_of(result.begin(), result.end(), [&](const Resolution& o) {
            return o.width == r.width && o.height == r.height;
        });

        if (!seen)
            result.push_back(r);
    }

    return result;
}

struct DisplayRequest {
    bool pending = false;
    bool fullscreen = false;
    int width = 0;
    int height = 0;
};

static DisplayRequest request;

static void displaySettings(GLFWwindow* window) {
    static const std::vector<Resolution> resolutions = supportedResolutions(glfwGetPrimaryMonitor());

    const bool fullscreen = glfwGetWindowMonitor(window) != nullptr;
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    int mode = fullscreen ? 1 : 0;

    if (ImGui::Combo("Mode", &mode, "Windowed\0Fullscreen (borderless)\0"))
        request = {true, mode == 1, width, height};

    ImGui::BeginDisabled(fullscreen);
    const std::string current = std::to_string(width) + " x " + std::to_string(height);

    if (ImGui::BeginCombo("Resolution", current.c_str())) {
        for (const Resolution& r : resolutions) {
            const std::string label = std::to_string(r.width) + " x " + std::to_string(r.height);

            if (ImGui::Selectable(label.c_str(), r.width == width && r.height == height))
                request = {true, false, r.width, r.height};
        }

        ImGui::EndCombo();
    }

    ImGui::EndDisabled();
}

void applyDisplayChanges(GLFWwindow* window) {
    static int windowedX = 100;
    static int windowedY = 100;
    static int windowedWidth = 1280;
    static int windowedHeight = 720;

    if (!request.pending)
        return;

    request.pending = false;
    const bool fullscreen = glfwGetWindowMonitor(window) != nullptr;

    if (request.fullscreen && !fullscreen) {
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

        // Don't minimize when focus moves elsewhere (e.g. a click on another monitor).
        glfwSetWindowAttrib(window, GLFW_AUTO_ICONIFY, GLFW_FALSE);
    } else if (!request.fullscreen && fullscreen) {
        glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
    } else if (!request.fullscreen) {
        glfwSetWindowSize(window, request.width, request.height);
    }

    // Some drivers reset the swap interval when the window changes monitor.
    glfwSwapInterval(vsync ? 1 : 0);
}

void initUI(GLFWwindow* window) {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true); // true = install its callbacks, chaining to existing ones
    ImGui_ImplOpenGL3_Init("#version 460");

    // Starting scale from the monitor: 1.0 for 1080p, ~1.25 for 1440p, ~2.0 for 4K, in 0.25 steps.
    int x, y, width, height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &x, &y, &width, &height);
    uiScale = std::max(1.0f, std::round(height / 1080.0f * 4.0f) / 4.0f);
    applyUiScale(uiScale);
}

void beginUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void endUI() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void shutdownUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

static entt::entity selected = entt::null;

// Entities whose parent is `parent` (entt::null for roots), in creation order. Views iterate newest first.
// ponytail: scans every entity per call, keep a children list in the scene if trees get large.
static std::vector<entt::entity> children(const Scene& scene, entt::entity parent) {
    std::vector<entt::entity> result;

    for (auto [entity, transform] : scene.registry.view<const Transform>().each()) {
        if (transform.parent == parent)
            result.push_back(entity);
    }

    std::sort(result.begin(), result.end());
    return result;
}

static void entityNode(const Scene& scene, entt::entity entity) {
    const std::vector<entt::entity> kids = children(scene, entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (kids.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (entity == selected)
        flags |= ImGuiTreeNodeFlags_Selected;

    // The entity id is the tree node's ID, so two entities with the same name ("Crate") stay distinct.
    const void* id = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(entt::to_integral(entity)));
    const bool open = ImGui::TreeNodeEx(id, flags, "%s", scene.registry.get<Name>(entity).value.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        selected = entity;

    if (open) {
        for (entt::entity kid : kids)
            entityNode(scene, kid);

        ImGui::TreePop();
    }
}

// The selected entity's components. Each gets its own ID scope, since labels like "Enabled" repeat.
static void inspector(Scene& scene) {
    entt::registry& registry = scene.registry;

    if (!registry.valid(selected)) {
        ImGui::TextDisabled("Select an entity to inspect it");
        return;
    }

    ImGui::SeparatorText(registry.get<Name>(selected).value.c_str());

    if (auto* transform = registry.try_get<Transform>(selected)) {
        ImGui::PushID("transform");
        ImGui::DragFloat3("Position", glm::value_ptr(transform->position), 0.05f);
        ImGui::DragFloat3("Rotation", glm::value_ptr(transform->rotation), 1.0f, 0.0f, 0.0f, "%.1f deg");
        ImGui::DragFloat3("Scale", glm::value_ptr(transform->scale), 0.01f);
        ImGui::PopID();
    }

    if (auto* rotator = registry.try_get<Rotator>(selected)) {
        ImGui::PushID("rotator");
        ImGui::SeparatorText("Rotator");
        ImGui::Checkbox("Enabled", &rotator->enabled);
        ImGui::DragFloat3("Speed", glm::value_ptr(rotator->degreesPerSecond), 1.0f, 0.0f, 0.0f, "%.1f deg/s");
        ImGui::PopID();
    }

    if (auto* light = registry.try_get<DirLight>(selected)) {
        ImGui::PushID("dirlight");
        ImGui::SeparatorText("Directional light");
        lightUI(*light);
        ImGui::PopID();
    }

    if (auto* light = registry.try_get<PointLight>(selected)) {
        ImGui::PushID("pointlight");
        ImGui::SeparatorText("Point light");
        lightUI(*light);
        ImGui::PopID();
    }

    if (auto* light = registry.try_get<SpotLight>(selected)) {
        ImGui::PushID("spotlight");
        ImGui::SeparatorText("Spot light (follows the camera)");
        lightUI(*light);
        ImGui::PopID();
    }

    if (auto* player = registry.try_get<Player>(selected)) {
        ImGui::PushID("player");
        ImGui::SeparatorText("Player");
        ImGui::DragFloat("Speed", &player->speed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Jump speed", &player->jumpSpeed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::PopID();
    }

    if (auto* health = registry.try_get<Health>(selected)) {
        ImGui::PushID("health");
        ImGui::SeparatorText("Health");
        ImGui::DragFloat("Current", &health->current, 1.0f, 0.0f, health->max);
        ImGui::DragFloat("Max", &health->max, 1.0f, 1.0f, 10000.0f);
        ImGui::PopID();
    }

    if (auto* enemy = registry.try_get<Enemy>(selected)) {
        ImGui::PushID("enemy");
        ImGui::SeparatorText("Enemy");
        ImGui::DragFloat("Speed", &enemy->speed, 0.1f, 0.0f, 50.0f, "%.1f u/s");
        ImGui::DragFloat("Radius", &enemy->radius, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Damage", &enemy->damage, 0.5f, 0.0f, 1000.0f, "%.1f /s");
        ImGui::PopID();
    }

    if (auto* spawner = registry.try_get<Spawner>(selected)) {
        ImGui::PushID("spawner");
        ImGui::SeparatorText("Spawner");
        ImGui::DragFloat("Interval", &spawner->interval, 0.05f, 0.05f, 60.0f, "%.2f s");
        ImGui::DragInt("Max enemies", &spawner->maxEnemies, 1.0f, 0, 2000);
        ImGui::DragFloat("Ring radius", &spawner->ringRadius, 0.1f, 1.0f, 40.0f);
        ImGui::PopID();
    }

    if (registry.all_of<MeshRenderer>(selected)) {
        ImGui::SeparatorText("Mesh renderer");
        ImGui::TextDisabled("Mesh with its own material");
    }

    if (auto* model = registry.try_get<ModelRenderer>(selected)) {
        ImGui::SeparatorText("Model renderer");
        ImGui::TextDisabled("%zu parts, materials from the file", model->model->parts.size());
    }
}

// Which panels are open, toggled from the View menu.
struct Panels {
    bool hierarchy = true;
    bool inspector = true;
    bool renderer = true;
    bool display = true;
    bool stats = true;
    bool demo = false; // ImGui's demo window: a catalog of every widget
};

static Panels panels;

// Position and size apply only the first time (afterwards imgui.ini remembers where the user put it).
// Begin returns false while the window is collapsed, but End must be called either way.
static bool beginPanel(const char* name, bool* open, ImVec2 pos, ImVec2 size) {
    ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    const bool visible = ImGui::Begin(name, open);

    // Negative width = the panel's width minus this much, which leaves room for labels at any panel size.
    ImGui::PushItemWidth(-ImGui::GetFontSize() * 8.0f);
    return visible;
}

static void endPanel() {
    ImGui::PopItemWidth();
    ImGui::End();
}

static void rendererPanel(Renderer& renderer) {
    ImGui::SeparatorText("Rendering");
    ImGui::Checkbox("Blinn-Phong", &renderer.blinn);
    ImGui::Checkbox("Wireframe", &renderer.wireframe);
    ImGui::Checkbox("Face culling", &renderer.faceCulling);
    ImGui::Checkbox("Skybox", &renderer.showSkybox);
    ImGui::Checkbox("Environment reflections", &renderer.reflections);
    ImGui::Checkbox("Fresnel", &renderer.fresnel);

    ImGui::SeparatorText("Post-processing");
    ImGui::Combo("Effect", &renderer.postEffect, "None\0Grayscale\0Invert\0Blur\0Sharpen\0Edge detection\0");
    ImGui::SliderFloat("Gamma", &renderer.gamma, 0.5f, 2.0f, "%.2f");

    ImGui::SeparatorText("Sun shadows");
    ImGui::Checkbox("Enabled", &renderer.shadows);

    const int resolutions[] = {1024, 2048, 4096};
    const std::string current = std::to_string(renderer.shadowResolution);

    if (ImGui::BeginCombo("Resolution", current.c_str())) {
        for (int r : resolutions) {
            if (ImGui::Selectable(std::to_string(r).c_str(), r == renderer.shadowResolution))
                renderer.shadowResolution = r;
        }

        ImGui::EndCombo();
    }

    ImGui::SliderFloat("Bias min", &renderer.shadowBiasMin, 0.0f, 0.01f, "%.4f");
    ImGui::SliderFloat("Bias max", &renderer.shadowBiasMax, 0.0f, 0.05f, "%.4f");
    ImGui::Checkbox("PCF (soft edges)", &renderer.pcf);
    ImGui::Checkbox("Cull front faces", &renderer.shadowCullFront);

    if (ImGui::TreeNode("Shadow map")) {
        // GL textures start at the bottom row, ImGui images at the top, so flip V.
        const float size = 200.0f * uiScale;
        ImGui::Image(static_cast<ImTextureID>(renderer.shadowMapTexture()), ImVec2(size, size), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        ImGui::TreePop();
    }
}

static void displayPanel(GLFWwindow* window) {
    displaySettings(window);

    if (ImGui::Checkbox("VSync", &vsync))
        glfwSwapInterval(vsync ? 1 : 0);

    // Applied when the slider is released: rescaling while dragging would resize the slider under the mouse.
    ImGui::SliderFloat("UI scale", &uiScale, 0.75f, 2.5f, "%.2f");

    if (ImGui::IsItemDeactivatedAfterEdit())
        applyUiScale(uiScale);
}

// Small overlay without a title bar, pinned to the bottom-left corner.
static void statsOverlay(const Scene& scene, const Renderer& renderer, float margin) {
    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(ImVec2(margin, io.DisplaySize.y - margin), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.4f);

    if (ImGui::Begin("Stats", &panels.stats, flags)) {
        ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
        ImGui::Text("%zu entities, %d lit draws", static_cast<size_t>(scene.registry.view<const Name>().size()), renderer.drawCalls);
        ImGui::Text("%zu enemies", static_cast<size_t>(scene.registry.view<const Enemy>().size()));

        for (auto [entity, health, player] : scene.registry.view<const Health, const Player>().each())
            ImGui::Text("Health %.0f / %.0f", health.current, health.max);

        ImGui::TextDisabled("Tab: UI mode | F1: fly camera");
    }

    ImGui::End();
}

void debugUI(GLFWwindow* window, Scene& scene, Renderer& renderer) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &panels.hierarchy);
            ImGui::MenuItem("Inspector", nullptr, &panels.inspector);
            ImGui::MenuItem("Renderer", nullptr, &panels.renderer);
            ImGui::MenuItem("Display", nullptr, &panels.display);
            ImGui::MenuItem("Stats", nullptr, &panels.stats);
            ImGui::Separator();
            ImGui::MenuItem("ImGui demo", nullptr, &panels.demo);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Default layout: scene panels on the left, settings on the right, below the menu bar.
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const float margin = 10.0f * uiScale;
    const float top = ImGui::GetFrameHeight() + margin;
    const float width = 340.0f * uiScale;
    const float hierarchyHeight = screen.y * 0.35f;
    const float right = screen.x - width - margin;

    if (panels.hierarchy) {
        if (beginPanel("Hierarchy", &panels.hierarchy, ImVec2(margin, top), ImVec2(width, hierarchyHeight))) {
            for (entt::entity root : children(scene, entt::null))
                entityNode(scene, root);
        }

        endPanel();
    }

    if (panels.inspector) {
        if (beginPanel("Inspector", &panels.inspector, ImVec2(margin, top + hierarchyHeight + margin), ImVec2(width, screen.y * 0.4f)))
            inspector(scene);

        endPanel();
    }

    if (panels.renderer) {
        if (beginPanel("Renderer", &panels.renderer, ImVec2(right, top), ImVec2(width, 0.0f)))
            rendererPanel(renderer);

        endPanel();
    }

    if (panels.display) {
        if (beginPanel("Display", &panels.display, ImVec2(right, screen.y * 0.65f), ImVec2(width, 0.0f)))
            displayPanel(window);

        endPanel();
    }

    if (panels.stats)
        statsOverlay(scene, renderer, margin);

    if (panels.demo)
        ImGui::ShowDemoWindow(&panels.demo);
}
