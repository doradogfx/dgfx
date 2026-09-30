#include "ui.h"

#include "renderer.h"
#include "scene.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <string>
#include <vector>

static bool vsync = true;

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

void debugPanel(GLFWwindow* window, Scene& scene, Renderer& renderer) {
    static bool showDemo = false;
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("dgfx");

    // Negative width = the panel's width minus this much, which leaves room for labels at any panel size.
    ImGui::PushItemWidth(-ImGui::GetFontSize() * 8.0f);

    ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
    ImGui::TextDisabled("Tab: toggle camera / UI mode");

    // Collapsing headers don't push an ID, so sections that reuse labels ("Enabled", "Resolution") get their
    // own ID scope. Tree nodes push one themselves.
    ImGui::PushID("lighting");
    if (ImGui::CollapsingHeader("Lighting")) {
        ImGui::Checkbox("Blinn-Phong", &renderer.blinn);
        ImGui::Checkbox("Orbit point lights", &scene.orbitLights);

        if (ImGui::TreeNode("Sun")) {
            lightUI(scene.sun);
            ImGui::TreePop();
        }

        for (int i = 0; i < kMaxPointLights; i++) {
            const std::string label = "Point " + std::to_string(i + 1);

            if (ImGui::TreeNode(label.c_str())) {
                lightUI(scene.points[i]);
                ImGui::TreePop();
            }
        }

        if (ImGui::TreeNode("Flashlight")) {
            lightUI(scene.flashlight);
            ImGui::TreePop();
        }
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
            ImGui::Image(static_cast<ImTextureID>(renderer.shadowMapTexture()), ImVec2(200.0f, 200.0f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            ImGui::TreePop();
        }
    }
    ImGui::PopID();

    ImGui::PushID("rendering");
    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::Checkbox("Wireframe", &renderer.wireframe);
        ImGui::Checkbox("Face culling", &renderer.faceCulling);
        ImGui::Checkbox("Skybox", &renderer.showSkybox);
        ImGui::Combo("Post effect", &renderer.postEffect, "None\0Grayscale\0Invert\0Blur\0Sharpen\0Edge detection\0");
        ImGui::SliderFloat("Gamma", &renderer.gamma, 0.5f, 2.0f, "%.2f");
    }
    ImGui::PopID();

    ImGui::PushID("display");
    if (ImGui::CollapsingHeader("Display")) {
        displaySettings(window);

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
