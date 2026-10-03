#include "core/application.h"

#include "ui/ui.h"

#include <GLFW/glfw3.h>

Application::Application()
    : renderer(window.framebufferSize().x, window.framebufferSize().y),
      input(window.handle()) {
    registerEngineComponents(components);
    initUI(window.handle());
}

Application::~Application() {
    shutdownUI();
}

void Application::run() {
    GLFWwindow* handle = window.handle();
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(handle)) {
        glfwPollEvents();

        // Seconds since the previous frame. Scaling movement by it keeps the speed the same at any frame rate.
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        if (glfwGetKey(handle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(handle, GLFW_TRUE);

        input.update();
        update(dt);

        const glm::ivec2 size = window.framebufferSize();

        // Minimized: the framebuffer is 0x0, so there's nothing to draw and the aspect ratio would divide by zero.
        // Sleep until the next event instead of spinning through empty frames.
        if (size.x == 0 || size.y == 0) {
            glfwWaitEvents();
            continue;
        }

        beginUI();
        debugUI(handle, scene, renderer, components, [this] { statsOverlay(); });

        renderer.render(scene, camera, size.x, size.y);
        endUI();

        glfwSwapBuffers(handle);
        applyDisplayChanges(handle);
    }
}
