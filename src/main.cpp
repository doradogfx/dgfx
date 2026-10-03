#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include "assets/assets.h"
#include "core/camera.h"
#include "core/input.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "scene/demo.h"
#include "scene/scene.h"
#include "ui/ui.h"

#include <cstdio>
#include <optional>

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

        Assets assets; // before the scene: components point into it
        Scene scene;
        Renderer renderer(width, height);

        Camera camera;

        // F2 switches between the game level (a Game exists) and the engine demo (none, so always fly camera).
        std::optional<Game> game;
        game.emplace(scene, assets);
        bool flyMode = false;

        Input input(window); // before initUI, so ImGui chains to its scroll callback
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

            input.update();

            if (input.pressed(GLFW_KEY_F1))
                flyMode = !flyMode;

            if (input.pressed(GLFW_KEY_F2)) {
                scene.registry.clear();
                camera = Camera();

                if (game) {
                    game.reset();
                    buildDemo(scene, assets);
                    camera.position = glm::vec3(0.0f, 2.0f, 6.0f);
                    camera.pitch = -15.0f;
                } else {
                    game.emplace(scene, assets);
                }
            }

            if (flyMode || !game)
                flyCamera(camera, input, dt);
            else
                game->update(scene, camera, input, dt);

            scene.update(dt, camera);

            glfwGetFramebufferSize(window, &width, &height);

            // Minimized: the framebuffer is 0x0, so there's nothing to draw and the aspect ratio would divide by zero.
            // Sleep until the next event instead of spinning through empty frames.
            if (width == 0 || height == 0) {
                glfwWaitEvents();
                continue;
            }

            beginUI();
            debugUI(window, scene, renderer);

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
