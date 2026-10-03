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

    if (pressed(GLFW_KEY_TAB))
        setCaptured(!captured);

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

void flyCamera(Camera& camera, const Input& input, float dt) {
    const float sensitivity = 0.1f; // degrees per pixel
    camera.turn(input.look.x * sensitivity, -input.look.y * sensitivity);
    camera.zoom(input.scroll * 2.0f); // 2 degrees of field of view per wheel notch

    float speed = 2.5f * dt; // units per second

    if (input.down(GLFW_KEY_LEFT_SHIFT))
        speed *= 4.0f;

    const glm::vec3 front = camera.front();
    const glm::vec3 right = camera.right();

    if (input.down(GLFW_KEY_W))
        camera.position += front * speed;
    if (input.down(GLFW_KEY_S))
        camera.position -= front * speed;
    if (input.down(GLFW_KEY_D))
        camera.position += right * speed;
    if (input.down(GLFW_KEY_A))
        camera.position -= right * speed;
    if (input.down(GLFW_KEY_SPACE))
        camera.position += Camera::worldUp * speed;
    if (input.down(GLFW_KEY_LEFT_CONTROL))
        camera.position -= Camera::worldUp * speed;
}
