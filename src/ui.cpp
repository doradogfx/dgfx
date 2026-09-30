#include "ui.h"

#include "renderer.h"
#include "scene.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>

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

void debugPanel(Scene& scene, Renderer& renderer) {
    static bool showDemo = false;
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("dgfx");
    ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
    ImGui::TextDisabled("Tab: toggle camera / UI mode");

    ImGui::SeparatorText("Lights");
    ImGui::Checkbox("Blinn-Phong", &renderer.blinn);
    ImGui::Checkbox("Orbit point lights", &scene.orbitLights);

    // Every light has widgets with the same labels; PushID keeps their IDs apart.
    if (ImGui::CollapsingHeader("Sun")) {
        ImGui::PushID("sun");
        lightUI(scene.sun);
        ImGui::PopID();
    }

    for (int i = 0; i < kMaxPointLights; i++) {
        const std::string label = "Point " + std::to_string(i + 1);

        if (ImGui::CollapsingHeader(label.c_str())) {
            ImGui::PushID(i);
            lightUI(scene.points[i]);
            ImGui::PopID();
        }
    }

    if (ImGui::CollapsingHeader("Flashlight")) {
        ImGui::PushID("flashlight");
        lightUI(scene.flashlight);
        ImGui::PopID();
    }

    ImGui::SeparatorText("Render");
    ImGui::Checkbox("Wireframe", &renderer.wireframe);
    ImGui::Checkbox("ImGui demo", &showDemo);

    ImGui::SeparatorText("Post");
    ImGui::Combo("Effect", &renderer.postEffect, "None\0Grayscale\0Invert\0Blur\0Sharpen\0Edge detection\0");
    ImGui::SliderFloat("Gamma", &renderer.gamma, 0.5f, 2.0f, "%.2f");
    ImGui::End();

    if (showDemo)
        ImGui::ShowDemoWindow(&showDemo);
}
