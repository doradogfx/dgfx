#include "core/input.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

CameraInput::CameraInput(GLFWwindow* window) : window(window) {
    // Raw motion skips the OS pointer acceleration, so the same hand movement always turns the same amount.
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // The scroll wheel has no state to poll, GLFW only reports it through a callback. The window's user
    // pointer is how the capture-less callback finds this object.
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, [](GLFWwindow* w, double, double yOffset) {
        static_cast<CameraInput*>(glfwGetWindowUserPointer(w))->scroll += yOffset;
    });

    setCaptured(true);
}

void CameraInput::setCaptured(bool value) {
    captured = value;
    glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Start measuring from here, or the distance moved while free would turn the camera in one jump.
    glfwGetCursorPos(window, &lastX, &lastY);
}

void CameraInput::update(Camera& camera, float dt) {
    // io.WantCapture* say whether ImGui is using the mouse/keyboard (hovering or typing in the panel).
    // They were updated by last frame's ImGui::NewFrame, which is recent enough.
    ImGuiIO& io = ImGui::GetIO();

    // Tab switches modes. Act only on the frame it goes down, or holding it would toggle every frame.
    // Not while typing in a text field, where Tab belongs to ImGui.
    const bool tabDown = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;

    if (tabDown && !tabWasDown && !io.WantCaptureKeyboard)
        setCaptured(!captured);

    tabWasDown = tabDown;

    // Give the cursor back when switching to another window. In UI mode, a click that isn't on the panel
    // goes back to camera mode.
    if (captured && !glfwGetWindowAttrib(window, GLFW_FOCUSED))
        setCaptured(false);
    else if (!captured && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse)
        setCaptured(true);

    // While captured, ImGui ignores the mouse so the hidden cursor can't click widgets.
    if (captured)
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    // Mouse look. Screen Y grows downward, so moving the mouse up gives a negative dy, which should tilt up.
    const float sensitivity = 0.1f; // degrees per pixel
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (captured)
        camera.turn(static_cast<float>(x - lastX) * sensitivity, static_cast<float>(lastY - y) * sensitivity);

    lastX = x;
    lastY = y;

    // 2 degrees of field of view per wheel notch. Over the panel, the scroll is ImGui's.
    if (!io.WantCaptureMouse)
        camera.zoom(static_cast<float>(scroll) * 2.0f);

    scroll = 0.0;

    // Skipped while typing in the panel, so text input doesn't fly the camera around.
    if (io.WantCaptureKeyboard)
        return;

    float speed = 2.5f * dt; // units per second

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 4.0f;

    const glm::vec3 front = camera.front();
    const glm::vec3 right = camera.right();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.position += front * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.position -= front * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.position += right * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.position -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.position += Camera::worldUp * speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.position -= Camera::worldUp * speed;
}
