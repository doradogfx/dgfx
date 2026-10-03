#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

// The OS window with its OpenGL 4.6 core context current. Throws if GLFW, the window or the GL loader fails.
class Window {
public:
    Window();
    ~Window();

    GLFWwindow* handle() const { return window; }
    glm::ivec2 framebufferSize() const;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

private:
    GLFWwindow* window = nullptr;
};
