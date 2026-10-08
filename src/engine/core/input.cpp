#include "core/input.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

Input::Input(GLFWwindow* window) : window(window) {
    // Raw motion skips the OS pointer acceleration, so the same hand movement always turns the same amount.
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // The scroll wheel has no state to poll, GLFW only reports it through a callback. The window's user
    // pointer is how the capture-less callback finds this object.
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, [](GLFWwindow* w, double, double yOffset) {
        static_cast<Input*>(glfwGetWindowUserPointer(w))->wheel += yOffset;
    });

    setCaptured(true);
}

void Input::setCaptured(bool value) {
    captured = value;
    glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Start measuring from here, or the distance moved while free would turn the camera in one jump.
    glfwGetCursorPos(window, &lastX, &lastY);
}

bool Input::down(int key) const {
    return keyboardFree && keys[key];
}

bool Input::pressed(int key) const {
    return keyboardFree && keys[key] && !lastKeys[key];
}

void Input::update() {
    // io.WantCapture* say whether ImGui is using the mouse/keyboard (hovering or typing in a panel).
    // They were updated by last frame's ImGui::NewFrame, which is recent enough.
    ImGuiIO& io = ImGui::GetIO();
    keyboardFree = !io.WantCaptureKeyboard;

    lastKeys = keys;

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
        keys[key] = glfwGetKey(window, key) == GLFW_PRESS;

#if DGFX_EDITOR
    if (pressed(GLFW_KEY_TAB)) // UI mode, for the debug panels
        setCaptured(!captured);
#endif

    // Give the cursor back when switching to another window. In UI mode, a click that isn't on a panel
    // goes back to captured mode.
    if (captured && !glfwGetWindowAttrib(window, GLFW_FOCUSED))
        setCaptured(false);
    else if (!captured && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse)
        setCaptured(true);

    // While captured, ImGui ignores the mouse so the hidden cursor can't click widgets.
    if (captured)
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    // Screen Y grows downward, so moving the mouse up gives a negative y.
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    look = captured ? glm::vec2(x - lastX, y - lastY) : glm::vec2(0.0f);
    lastX = x;
    lastY = y;

    // Over a panel, the scroll is ImGui's.
    scroll = io.WantCaptureMouse ? 0.0f : static_cast<float>(wheel);
    wheel = 0.0;
}

void flyCamera(Scene& scene, const Input& input, float dt) {
    const entt::entity entity = scene.camera();

    if (entity == entt::null)
        return;

    Transform& transform = scene.registry.get<Transform>(entity);

    // Rotation y turns left for positive angles, x looks up. Pitch stops short of straight up/down: at 90 degrees
    // the view direction would be parallel to world up, and the view flips.
    const float sensitivity = 0.1f; // degrees per pixel
    transform.rotation.y -= input.look.x * sensitivity;
    transform.rotation.x = glm::clamp(transform.rotation.x - input.look.y * sensitivity, -89.0f, 89.0f);

    float speed = 2.5f * dt; // units per second

    if (input.down(GLFW_KEY_LEFT_SHIFT))
        speed *= 4.0f;

    const glm::vec3 front = scene.forward(entity);
    const glm::vec3 right = glm::normalize(glm::cross(front, kWorldUp));

    if (input.down(GLFW_KEY_W))
        transform.position += front * speed;
    if (input.down(GLFW_KEY_S))
        transform.position -= front * speed;
    if (input.down(GLFW_KEY_D))
        transform.position += right * speed;
    if (input.down(GLFW_KEY_A))
        transform.position -= right * speed;
    if (input.down(GLFW_KEY_SPACE))
        transform.position += kWorldUp * speed;
    if (input.down(GLFW_KEY_LEFT_CONTROL))
        transform.position -= kWorldUp * speed;
}
