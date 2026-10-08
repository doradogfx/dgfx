#include "core/application.h"

#include "core/paths.h"
#include "scene/scene_io.h"
#include "ui/ui.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <vector>

Application::Application()
    : renderer(window.framebufferSize().x, window.framebufferSize().y),
      input(window.handle()) {
    registerEngineComponents(components);
    initUI(window.handle());
}

// The .json files in scenes/, by name.
static std::vector<std::string> sceneFiles() {
    std::vector<std::string> names;

    for (const auto& entry : std::filesystem::directory_iterator(assetRoot() + "scenes/")) {
        if (entry.path().extension() == ".json")
            names.push_back(entry.path().filename().string());
    }

    std::sort(names.begin(), names.end());
    return names;
}

void Application::loadScene(const std::string& file) {
    scene.registry = entt::registry(); // fresh, so ids count up from 0 in creation order (clear() would recycle them)
    ::loadScene(scene, assets, components, assetRoot() + "scenes/" + file);
    currentScene = file;
}

void Application::saveScene(const std::string& file) const {
    ::saveScene(scene, assets, components, assetRoot() + "scenes/" + file);
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

        if (!pendingScene.empty()) {
            openScene(pendingScene);
            pendingScene.clear();
        }

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

#if DGFX_EDITOR
        UiContext ui{
            scene, assets, renderer, components, currentScene,
            [this] {
                try {
                    saveScene(currentScene);
                    return true;
                } catch (const std::exception& e) {
                    std::fprintf(stderr, "%s\n", e.what());
                    return false;
                }
            },
            [this] { statsOverlay(); },
            sceneFiles,
            [this](const std::string& file) { pendingScene = file; },
        };

        debugUI(handle, ui);
#endif
        gameUI();

        renderer.render(scene, camera, size.x, size.y);
        endUI();

        glfwSwapBuffers(handle);
        applyDisplayChanges(handle);
    }
}
