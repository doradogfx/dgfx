#include "ui/panels.h"

#include "ui/ui.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

static bool vsync = true;
static float scale = 1.0f;

// Scales text and every padding/spacing/size. ScaleAllSizes multiplies the current values,
// so it's applied to a fresh default style each time instead of compounding.
static void applyUiScale(float factor) {
    ImGuiStyle style;
    ImGui::StyleColorsDark(&style);
    style.ScaleAllSizes(factor);
    style.FontScaleMain = factor;
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

void displayPanel(GLFWwindow* window) {
    displaySettings(window);

    if (ImGui::Checkbox("VSync", &vsync))
        glfwSwapInterval(vsync ? 1 : 0);

    // Applied when the slider is released: rescaling while dragging would resize the slider under the mouse.
    ImGui::SliderFloat("UI scale", &scale, 0.75f, 2.5f, "%.2f");

    if (ImGui::IsItemDeactivatedAfterEdit())
        applyUiScale(scale);
}

float uiScale() {
    return scale;
}

void initUiScale() {
    // Starting scale from the monitor: 1.0 for 1080p, ~1.25 for 1440p, ~2.0 for 4K, in 0.25 steps.
    int x, y, width, height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &x, &y, &width, &height);
    scale = std::max(1.0f, std::round(height / 1080.0f * 4.0f) / 4.0f);
    applyUiScale(scale);
}
