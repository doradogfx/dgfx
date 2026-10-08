#include "ui/ui.h"

#include "renderer/renderer.h"
#include "scene/scene.h"
#include "ui/panels.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>

void initUI(GLFWwindow* window) {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true); // true = install its callbacks, chaining to existing ones
    ImGui_ImplOpenGL3_Init("#version 460");
    initUiScale();
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

// Which panels are open, toggled from the View menu.
struct Panels {
    bool hierarchy = true;
    bool inspector = true;
    bool renderer = true;
    bool display = true;
    bool stats = true;
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

// Small overlay without a title bar, pinned to the bottom-left corner.
static void statsOverlay(const UiContext& context, float margin) {
    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(ImVec2(margin, io.DisplaySize.y - margin), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.4f);

    if (ImGui::Begin("Stats", &panels.stats, flags)) {
        ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
        ImGui::Text("%zu entities, %d objects in %d draws", static_cast<size_t>(context.scene.registry.view<const Name>().size()), context.renderer.instances, context.renderer.drawCalls);

        if (context.statsExtra)
            context.statsExtra();

        ImGui::TextDisabled("Tab: toggle camera / UI mode");
    }

    ImGui::End();
}

void debugUI(GLFWwindow* window, UiContext& context) {
    static double savedAt = -100.0;
    static bool saveFailed = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("Open scene", static_cast<bool>(context.listScenes) && static_cast<bool>(context.openScene))) {
                for (const std::string& file : context.listScenes()) {
                    if (ImGui::MenuItem(file.c_str(), nullptr, file == context.sceneName))
                        context.openScene(file);
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Save scene", nullptr, false, !context.sceneName.empty() && static_cast<bool>(context.save))) {
                saveFailed = !context.save();
                savedAt = ImGui::GetTime();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &panels.hierarchy);
            ImGui::MenuItem("Inspector", nullptr, &panels.inspector);
            ImGui::MenuItem("Renderer", nullptr, &panels.renderer);
            ImGui::MenuItem("Display", nullptr, &panels.display);
            ImGui::MenuItem("Stats", nullptr, &panels.stats);
            ImGui::EndMenu();
        }

        // The loaded scene on the right, with the result of a save for a few seconds.
        std::string label = context.sceneName;

        if (!label.empty() && ImGui::GetTime() - savedAt < 3.0)
            label += saveFailed ? "  (save failed, see the console)" : "  (saved)";

        const float labelWidth = ImGui::CalcTextSize(label.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - labelWidth - ImGui::GetStyle().ItemSpacing.x * 2.0f);
        ImGui::TextDisabled("%s", label.c_str());

        ImGui::EndMainMenuBar();
    }

    // Default layout: scene panels on the left, settings on the right, below the menu bar.
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const float margin = 10.0f * uiScale();
    const float top = ImGui::GetFrameHeight() + margin;
    const float width = 340.0f * uiScale();
    const float hierarchyHeight = screen.y * 0.35f;
    const float right = screen.x - width - margin;

    if (panels.hierarchy) {
        if (beginPanel("Hierarchy", &panels.hierarchy, ImVec2(margin, top), ImVec2(width, hierarchyHeight)))
            hierarchyPanel(context);

        endPanel();
    }

    if (panels.inspector) {
        if (beginPanel("Inspector", &panels.inspector, ImVec2(margin, top + hierarchyHeight + margin), ImVec2(width, screen.y * 0.4f)))
            inspectorPanel(context);

        endPanel();
    }

    if (panels.renderer) {
        if (beginPanel("Renderer", &panels.renderer, ImVec2(right, top), ImVec2(width, 0.0f)))
            rendererPanel(context.renderer);

        endPanel();
    }

    if (panels.display) {
        if (beginPanel("Display", &panels.display, ImVec2(right, screen.y * 0.65f), ImVec2(width, 0.0f)))
            displayPanel(window);

        endPanel();
    }

    if (panels.stats)
        statsOverlay(context, margin);
}
