#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include "camera.h"
#include "light.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>
#include <cmath>
#include <cstdio>

// How a surface reflects light. Mirrors the Material struct and maps in lit.frag.
// The maps give per-pixel colors that the tints multiply; a null map means "none" (a white texture is bound instead).
struct Material {
    const Texture* diffuseMap;
    const Texture* specularMap;
    glm::vec3 diffuse;  // tint for the diffuse map, or the whole color when there's no map
    glm::vec3 specular; // tint for the specular map, or the whole highlight color when there's no map
    float shininess;
};

// Colors picked by eye are sRGB (gamma-encoded); lighting math needs linear values.
// 2.2 approximates the exact sRGB curve closely enough for picked colors.
static glm::vec3 srgb(glm::vec3 color) {
    return glm::pow(color, glm::vec3(2.2f));
}

// Plastic: the surface color everywhere, plus a white-ish highlight (the light's color, not the surface's).
static Material plastic(glm::vec3 color, float shininess = 128.0f) {
    return {nullptr, nullptr, srgb(color), glm::vec3(0.5f), shininess};
}

// Rubber: the surface color, almost no highlight, and what little there is is wide and dull.
static Material rubber(glm::vec3 color) {
    return {nullptr, nullptr, srgb(color), glm::vec3(0.1f), 8.0f};
}

static PointLight coloredLight(glm::vec3 position, glm::vec3 color) {
    PointLight light;
    light.position = position;
    light.ambient = srgb(color) * 0.01f;
    light.diffuse = srgb(color);
    light.specular = srgb(color);
    return light;
}

int main() {
    glfwSetErrorCallback([](int code, const char* desc) {
        std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
    });

    if (!glfwInit())
        return 1;

    // Ask for a modern core-profile context (no deprecated fixed-function API).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE); // needed for GL_FRAMEBUFFER_SRGB on the default framebuffer

    GLFWwindow* window = glfwCreateWindow(1280, 720, "dgfx", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        return 1;
    }

    // GL calls go to the context that is "current" on this thread.
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // GL functions live in the driver; GLAD looks up their addresses at runtime.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "Failed to load OpenGL functions\n");
        glfwTerminate();
        return 1;
    }

    std::printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    {
        // Lit objects and the lamp share the vertex shader; only how the pixels are colored differs.
        Shader lit(SHADER_DIR "lit.vert", SHADER_DIR "lit.frag");
        Shader lamp(SHADER_DIR "lit.vert", SHADER_DIR "light.frag");

        DirLight sun;
        sun.ambient = glm::vec3(0.01f);
        sun.diffuse = srgb({0.3f, 0.28f, 0.25f}); // dim and slightly warm, so the point lights stand out

        PointLight points[kMaxPointLights] = {
            coloredLight({ 2.0f, 1.0f,  1.5f}, {1.0f, 0.2f, 0.2f}),
            coloredLight({-2.0f, 1.0f,  1.5f}, {0.2f, 1.0f, 0.2f}),
            coloredLight({-1.5f, 1.2f, -2.0f}, {0.2f, 0.3f, 1.0f}),
            coloredLight({ 2.0f, 1.5f, -2.0f}, {1.0f, 1.0f, 1.0f}),
        };

        SpotLight flashlight; // follows the camera, off by default

        bool blinn = true;
        bool orbitLights = true; // rotates the point lights around the scene's vertical axis

        bool wireframe = false;
        bool showDemo = false; // ImGui demo window

        const Mesh cube = makeCube();
        const Mesh sphere = makeSphere();

        const Texture white(glm::vec3(1.0f));
        const Texture crateDiffuse(TEXTURE_DIR "container2.png", true);
        const Texture crateSpecular(TEXTURE_DIR "container2_specular.png", false);

        struct Object {
            const Mesh* mesh;
            glm::vec3 position;
            float yaw;
            glm::vec3 scale;
            Material material;
        };

        // Gold, from the classic OpenGL material tables (devernay.free.fr/cours/opengl/materials.html).
        // Unlike plastic, a metal's highlight takes the metal's own color.
        const Material gold = {nullptr, nullptr, srgb({0.75164f, 0.60648f, 0.22648f}), srgb({0.628281f, 0.555802f, 0.366065f}), 51.2f * 4.0f}; // table value is for Phong

        // Wooden crate with a steel rim. The specular map is black over the wood and bright over the metal,
        // so only the rim catches highlights. White tints: the maps alone decide the colors.
        const Material crate = {&crateDiffuse, &crateSpecular, glm::vec3(1.0f), glm::vec3(1.0f), 128.0f};

        const Object objects[] = {
            {&cube,   { 0.0f, -0.55f,  0.0f},  0.0f, {10.0f, 0.1f, 10.0f}, rubber({0.6f, 0.6f, 0.6f})},            // floor, top surface at y = -0.5
            {&sphere, { 0.0f,  0.0f,   0.0f},  0.0f, { 1.0f, 1.0f,  1.0f}, plastic({1.0f, 0.5f, 0.3f})},           // orange
            {&sphere, { 2.0f,  0.0f,  -1.0f}, 30.0f, { 1.0f, 1.0f,  1.0f}, plastic({0.3f, 0.5f, 1.0f}, 512.0f)},   // blue, glossy
            {&sphere, {-1.5f, -0.25f,  1.0f},  0.0f, { 0.5f, 0.5f,  0.5f}, rubber({0.4f, 0.9f, 0.4f})},            // small green
            {&sphere, { 1.5f,  0.0f,   1.0f}, 45.0f, { 1.0f, 1.0f,  1.0f}, plastic({0.7f, 0.05f, 0.87f})},         // purple
            {&sphere, {-2.2f,  0.0f,  -1.2f}, 15.0f, { 1.0f, 1.0f,  1.0f}, gold},                                  // gold
            {&cube,   { 0.0f,  0.0f,   2.4f}, 25.0f, { 1.0f, 1.0f,  1.0f}, crate},                                 // crate, front
            {&cube,   {-3.2f,  0.0f,   0.6f}, 10.0f, { 1.0f, 1.0f,  1.0f}, crate},                                 // crate, left
        };

        glEnable(GL_DEPTH_TEST);

        Camera camera;
        camera.position = glm::vec3(0.0f, 2.0f, 6.0f);
        camera.pitch = -15.0f;

        // Raw motion skips the OS pointer acceleration, so the same hand movement always turns the same amount.
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        // The scroll wheel has no current state to poll, GLFW only reports it through a callback.
        // The callback adds to this, the loop consumes it. The window's user pointer is how the
        // capture-less callback finds it.
        double scroll = 0.0;
        glfwSetWindowUserPointer(window, &scroll);
        glfwSetScrollCallback(window, [](GLFWwindow* w, double, double yOffset) {
            *static_cast<double*>(glfwGetWindowUserPointer(w)) += yOffset;
        });

        // ImGui. Must come after our callbacks: with install_callbacks = true it installs its own and chains
        // to the ones already set, so both ImGui and our scroll zoom see the events.
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 460");
        ImGuiIO& io = ImGui::GetIO();

        // Two input modes. Camera mode: cursor hidden and locked to the window, so the mouse can turn the camera
        // forever without hitting the screen edge. UI mode: normal cursor, for clicking the ImGui panel.
        // While the cursor is captured ImGui is told to ignore the mouse, so the hidden cursor can't click widgets.
        bool cursorCaptured = false;
        double lastX, lastY; // mouse look works on how far the cursor moved since last frame

        auto setCaptured = [&](bool captured) {
            cursorCaptured = captured;
            glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

            if (captured)
                io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            else
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

            // Start measuring from here, or the distance moved while free would turn the camera in one jump.
            glfwGetCursorPos(window, &lastX, &lastY);
        };

        setCaptured(true);
        bool tabWasDown = false;

        double lastTime = glfwGetTime();

        // The main loop: poll OS events, render into the back buffer, present it.
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Seconds since the previous frame. Scaling movement by it keeps the speed the same at any frame rate.
            const double now = glfwGetTime();
            const float dt = static_cast<float>(now - lastTime);
            lastTime = now;

            // Poll the key's current state; setting the close flag ends the loop on its next check.
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            // io.WantCapture* say whether ImGui is using the mouse/keyboard (hovering or typing in the panel).
            // They were updated by last frame's ImGui::NewFrame, which is recent enough.

            // Tab switches between camera and UI mode. Act only on the frame it goes down, or holding it would
            // toggle every frame. Not while typing in a text field, where Tab belongs to ImGui.
            const bool tabDown = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;

            if (tabDown && !tabWasDown && !io.WantCaptureKeyboard)
                setCaptured(!cursorCaptured);

            tabWasDown = tabDown;

            // Give the cursor back when switching to another window. In UI mode, a click that isn't on the panel
            // goes back to camera mode.
            if (cursorCaptured && !glfwGetWindowAttrib(window, GLFW_FOCUSED))
                setCaptured(false);
            else if (!cursorCaptured && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse)
                setCaptured(true);

            // Mouse look: horizontal movement turns left/right (yaw), vertical tilts up/down (pitch).
            // Screen Y grows downward, so moving the mouse up gives a negative dy, which should tilt up: hence the minus.
            const float sensitivity = 0.1f; // degrees per pixel
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            if (cursorCaptured)
                camera.turn(static_cast<float>(x - lastX) * sensitivity, static_cast<float>(lastY - y) * sensitivity);

            lastX = x;
            lastY = y;

            // Scroll up zooms in, 2 degrees of field of view per wheel notch. Over the panel, the scroll is ImGui's.
            if (!io.WantCaptureMouse)
                camera.zoom(static_cast<float>(scroll) * 2.0f);

            scroll = 0.0;

            // WASD moves along where the camera looks (flying, so looking up and pressing W goes up).
            // Space/Left Ctrl move straight up/down in the world. Holding Left Shift moves 4x faster.
            // Skipped while typing in the panel, so text input doesn't fly the camera around.
            if (!io.WantCaptureKeyboard) {
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

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            // Minimized: the framebuffer is 0x0, so there's nothing to draw and the aspect ratio would divide by zero.
            // Sleep until the next event instead of spinning through empty frames.
            if (width == 0 || height == 0) {
                glfwWaitEvents();
                continue;
            }

            // Build this frame's UI. ImGui is immediate mode: the panel is described from scratch every frame, and
            // each widget edits the variable it's given in place, so a changed value is used right away below.
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("dgfx");
            ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
            ImGui::TextDisabled("Tab: toggle camera / UI mode");

            ImGui::SeparatorText("Lights");
            ImGui::Checkbox("Blinn-Phong", &blinn);
            ImGui::Checkbox("Orbit point lights", &orbitLights);

            // Every light has widgets with the same labels; PushID keeps their IDs apart.
            if (ImGui::CollapsingHeader("Sun")) {
                ImGui::PushID("sun");
                lightUI(sun);
                ImGui::PopID();
            }

            for (int i = 0; i < kMaxPointLights; i++) {
                const std::string label = "Point " + std::to_string(i + 1);

                if (ImGui::CollapsingHeader(label.c_str())) {
                    ImGui::PushID(i);
                    lightUI(points[i]);
                    ImGui::PopID();
                }
            }

            if (ImGui::CollapsingHeader("Flashlight")) {
                ImGui::PushID("flashlight");
                lightUI(flashlight);
                ImGui::PopID();
            }

            ImGui::SeparatorText("Render");
            ImGui::Checkbox("Wireframe", &wireframe);
            ImGui::Checkbox("ImGui demo", &showDemo);
            ImGui::End();

            if (showDemo)
                ImGui::ShowDemoWindow(&showDemo);

            glViewport(0, 0, width, height);
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

            // The scene is computed in linear space; the GPU encodes to sRGB when writing each pixel (and the clear).
            glEnable(GL_FRAMEBUFFER_SRGB);

            glClearColor(0.01f, 0.01f, 0.01f, 1.0f); // linear, displays as a dark grey
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Camera's vertical field of view, window aspect ratio (so nothing stretches), and near/far clip planes.
            // A Vulkan backend would need a different projection here: depth range 0..1 instead of -1..1, and Y flipped.
            const glm::mat4 projection = glm::perspective(glm::radians(camera.fov), static_cast<float>(width) / height, 0.1f, 100.0f);
            const glm::mat4 view = camera.view();

            // The panel edits base positions; orbiting rotates copies of them around the Y axis.
            PointLight worldPoints[kMaxPointLights];
            const glm::mat4 orbit = glm::rotate(glm::mat4(1.0f), orbitLights ? static_cast<float>(now) * 0.5f : 0.0f, Camera::worldUp);

            for (int i = 0; i < kMaxPointLights; i++) {
                worldPoints[i] = points[i];
                worldPoints[i].position = glm::vec3(orbit * glm::vec4(points[i].position, 1.0f));
            }

            flashlight.position = camera.position;
            flashlight.direction = camera.front();

            lit.use();
            lit.setMat4("projection", projection);
            lit.setMat4("view", view);
            lit.setVec3("viewPos", camera.position);
            lit.setBool("blinn", blinn);
            setLights(lit, sun, worldPoints, flashlight);

            for (const Object& obj : objects) {
                // Right to left: scale, then rotate around the object's center, then move it into place.
                glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.position);
                model = glm::rotate(model, glm::radians(obj.yaw), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, obj.scale);

                // Normals can't just use the model matrix: a non-uniform scale (like the flattened floor) would
                // tilt them so they no longer point straight out of the surface. The inverse transpose undoes the
                // scale's effect on direction while keeping rotation. mat3 drops translation, directions don't move.
                const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

                lit.setMat4("model", model);
                lit.setMat3("normalMatrix", normalMatrix);
                // Maps go to the units the shader's samplers read (layout binding 0 and 1).
                (obj.material.diffuseMap ? obj.material.diffuseMap : &white)->bind(0);
                (obj.material.specularMap ? obj.material.specularMap : &white)->bind(1);
                lit.setVec3("material.diffuse", obj.material.diffuse);
                lit.setVec3("material.specular", obj.material.specular);
                lit.setFloat("material.shininess", obj.material.shininess);

                obj.mesh->draw();
            }

            // A small sphere per enabled point light, in its color.
            lamp.use();
            lamp.setMat4("projection", projection);
            lamp.setMat4("view", view);

            for (const PointLight& p : worldPoints) {
                if (!p.enabled)
                    continue;

                lamp.setMat4("model", glm::scale(glm::translate(glm::mat4(1.0f), p.position), glm::vec3(0.15f)));
                lamp.setVec3("lightColor", p.diffuse);
                sphere.draw();
            }

            // UI last, on top of the scene.
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_FRAMEBUFFER_SRGB);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        // Before the GL context goes away: the OpenGL backend frees its own GL objects.
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
