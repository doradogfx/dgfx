#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include "core/camera.h"
#include "core/input.h"
#include "renderer/renderer.h"
#include "scene/scene.h"
#include "ui/ui.h"

#include <cstdio>

int main() {
    glfwSetErrorCallback([](int code, const char* desc) {
        std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
    });

    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE); // needed for GL_FRAMEBUFFER_SRGB on the default framebuffer

    // Open at 80% of the primary monitor's work area (the desktop minus the taskbar), centered, so the
    // window suits any monitor size.
    int areaX, areaY, areaWidth, areaHeight;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &areaX, &areaY, &areaWidth, &areaHeight);
    const int windowWidth = areaWidth * 4 / 5;
    const int windowHeight = areaHeight * 4 / 5;
    glfwWindowHint(GLFW_POSITION_X, areaX + (areaWidth - windowWidth) / 2);
    glfwWindowHint(GLFW_POSITION_Y, areaY + (areaHeight - windowHeight) / 2);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "dgfx", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // GL functions live in the driver; GLAD looks up their addresses at runtime.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "Failed to load OpenGL functions\n");
        glfwTerminate();
        return 1;
    }

    std::printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    // Scoped so everything holding GL objects is destroyed while the context still exists.
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        Scene scene;
        Renderer renderer(width, height);

        Camera camera;
        camera.position = glm::vec3(0.0f, 2.0f, 6.0f);
        camera.pitch = -15.0f;

        CameraInput input(window); // before initUI, so ImGui chains to its scroll callback
        initUI(window);

        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Seconds since the previous frame. Scaling movement by it keeps the speed the same at any frame rate.
            const double now = glfwGetTime();
            const float dt = static_cast<float>(now - lastTime);
            lastTime = now;

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            input.update(camera, dt);
            scene.update(dt, camera);

            glfwGetFramebufferSize(window, &width, &height);

            // Minimized: the framebuffer is 0x0, so there's nothing to draw and the aspect ratio would divide by zero.
            // Sleep until the next event instead of spinning through empty frames.
            if (width == 0 || height == 0) {
                glfwWaitEvents();
                continue;
            }

            beginUI();
            debugPanel(window, scene, renderer);

            renderer.render(scene, camera, width, height);
            endUI();

            glfwSwapBuffers(window);
            applyDisplayChanges(window);
        }

        shutdownUI();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
