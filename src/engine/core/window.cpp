#include "core/window.h"

#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include <cstdio>
#include <stdexcept>

Window::Window() {
    glfwSetErrorCallback([](int code, const char* desc) {
        std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
    });

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE); // needed for GL_FRAMEBUFFER_SRGB on the default framebuffer

    // Open at 80% of the primary monitor's work area (the desktop minus the taskbar), centered, so the
    // window suits any monitor size.
    int areaX, areaY, areaWidth, areaHeight;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &areaX, &areaY, &areaWidth, &areaHeight);
    const int width = areaWidth * 4 / 5;
    const int height = areaHeight * 4 / 5;
    glfwWindowHint(GLFW_POSITION_X, areaX + (areaWidth - width) / 2);
    glfwWindowHint(GLFW_POSITION_Y, areaY + (areaHeight - height) / 2);

    window = glfwCreateWindow(width, height, "dgfx", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create the window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // GL functions live in the driver; GLAD looks up their addresses at runtime.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL functions");
    }

    std::printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

glm::ivec2 Window::framebufferSize() const {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return {width, height};
}
