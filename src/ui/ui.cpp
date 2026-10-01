#include "ui/ui.h"

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

    if (registry.all_of<MeshRenderer>(selected)) {
        ImGui::SeparatorText("Mesh renderer");
        ImGui::TextDisabled("Mesh with its own material");
    }

    if (auto* model = registry.try_get<ModelRenderer>(selected)) {
        ImGui::SeparatorText("Model renderer");
        ImGui::TextDisabled("%zu parts, materials from the file", model->model->parts.size());
    }
}

void debugPanel(GLFWwindow* window, Scene& scene, Renderer& renderer) {
    static bool showDemo = false;
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.0f * uiScale, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("dgfx");

    // Negative width = the panel's width minus this much, which leaves room for labels at any panel size.
    ImGui::PushItemWidth(-ImGui::GetFontSize() * 8.0f);

    ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
    ImGui::TextDisabled("Tab: toggle camera / UI mode");

    // Collapsing headers don't push an ID, so sections that reuse labels ("Enabled", "Resolution") get their
    // own ID scope. Tree nodes push one themselves.
    ImGui::PushID("scene");
    if (ImGui::CollapsingHeader("Scene")) {
        for (entt::entity root : children(scene, entt::null))
            entityNode(scene, root);

        inspector(scene);
    }
    ImGui::PopID();

    ImGui::PushID("shadows");
    if (ImGui::CollapsingHeader("Shadows")) {
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
    ImGui::PopID();

    ImGui::PushID("rendering");
    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::Checkbox("Blinn-Phong", &renderer.blinn);
        ImGui::Checkbox("Wireframe", &renderer.wireframe);
        ImGui::Checkbox("Face culling", &renderer.faceCulling);
        ImGui::Checkbox("Skybox", &renderer.showSkybox);
        ImGui::Checkbox("Environment reflections", &renderer.reflections);
        ImGui::Checkbox("Fresnel", &renderer.fresnel);
        ImGui::Combo("Post effect", &renderer.postEffect, "None\0Grayscale\0Invert\0Blur\0Sharpen\0Edge detection\0");
        ImGui::SliderFloat("Gamma", &renderer.gamma, 0.5f, 2.0f, "%.2f");
    }
    ImGui::PopID();

    ImGui::PushID("display");
    if (ImGui::CollapsingHeader("Display")) {
        displaySettings(window);

        // Applied when the slider is released: rescaling while dragging would resize the slider under the mouse.
        ImGui::SliderFloat("UI scale", &uiScale, 0.75f, 2.5f, "%.2f");

        if (ImGui::IsItemDeactivatedAfterEdit())
            applyUiScale(uiScale);

        if (ImGui::Checkbox("VSync", &vsync))
            glfwSwapInterval(vsync ? 1 : 0);
    }
    ImGui::PopID();

    if (ImGui::CollapsingHeader("Debug"))
        ImGui::Checkbox("ImGui demo", &showDemo);

    ImGui::PopItemWidth();
    ImGui::End();

    if (showDemo)
        ImGui::ShowDemoWindow(&showDemo);
}
