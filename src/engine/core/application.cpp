#include "core/application.h"

#include "core/paths.h"
#include "scene/scene_io.h"

#include <GLFW/glfw3.h>

Application::Application()
    : input(window.handle()),
      ui(window.handle()) {
    registerEngineComponents(components);
}

void Application::openScene(const std::string& file) {
    loadScene(scene, assets, components, assetRoot() + "scenes/" + file);
    currentScene = file;
    sceneOpened();
}

void Application::saveScene() const {
    ::saveScene(scene, assets, components, assetRoot() + "scenes/" + currentScene);
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

        // 1. The game logic.
        input.update();

#if DGFX_EDITOR
        if (input.pressed(GLFW_KEY_F1))
            flyMode = !flyMode;
#endif

        if (flyMode)
            flyCamera(scene, input, dt);
        else
            update(dt);

        const glm::ivec2 size = window.framebufferSize();

        // Minimized: the framebuffer is 0x0, so there's nothing to draw and the aspect ratio would divide by zero.
        // Sleep until the next event instead of spinning through empty frames.
        if (size.x == 0 || size.y == 0) {
            glfwWaitEvents();
            continue;
        }

        // 2. The UI. ImGui only collects it here, endFrame draws it over the scene.
        ui.beginFrame();
        std::string pickedScene;

#if DGFX_EDITOR
        pickedScene = debugUI(*this);
#endif
        gameUI();

        // 3. The frame.
        renderer.render(scene, size.x, size.y);
        ui.endFrame();
        glfwSwapBuffers(handle);

        // 4. Between frames: changes that must not happen while a frame uses the window or the scene.
        applyDisplayChanges(handle);

        if (!pickedScene.empty())
            openScene(pickedScene);
    }
}
