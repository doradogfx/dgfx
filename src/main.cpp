#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include "camera.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
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

// Plastic: the surface color everywhere, plus a white-ish highlight (the light's color, not the surface's).
static Material plastic(glm::vec3 color, float shininess = 128.0f) {
    return {nullptr, nullptr, color, glm::vec3(0.5f), shininess};
}

// Rubber: the surface color, almost no highlight, and what little there is is wide and dull.
static Material rubber(glm::vec3 color) {
    return {nullptr, nullptr, color, glm::vec3(0.1f), 8.0f};
}

enum LightType { Directional = 0, Point = 1, Spot = 2 };

struct Light {
    int type = Point;
    glm::vec3 direction{-0.2f, -1.0f, -0.3f}; // directional: sun from above, slightly angled
    glm::vec3 ambient{0.1f};                  // 0 = faces turned away from the light go pitch black
    glm::vec3 diffuse{1.0f};                  // the light's color; try (1, 0.3, 0.3) for a red light
    glm::vec3 specular{1.0f};                 // 0 = no highlights on anything
    float constant = 1.0f;                    // attenuation, see kAttenuationPresets
    float linear = 0.22f;
    float quadratic = 0.20f;
    float innerAngle = 12.5f;                 // spot cone, degrees: full brightness inside
    float outerAngle = 17.5f;                 // degrees: dark outside, soft fade between inner and outer
};

// Attenuation factors that make a light reach about `range` units, from the widely used Ogre3D table.
struct AttenuationPreset {
    const char* name;
    float linear;
    float quadratic;
};

static const AttenuationPreset kAttenuationPresets[] = {
    {"7", 0.7f, 1.8f}, {"13", 0.35f, 0.44f}, {"20", 0.22f, 0.20f},
    {"32", 0.14f, 0.07f}, {"50", 0.09f, 0.032f}, {"100", 0.045f, 0.0075f},
};

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

        Light light;
        bool blinn = true;      // false = classic Phong, to compare the highlights
        bool orbitLight = true; // point light: false = stays still at its starting position

        bool wireframe = false;
        bool showDemo = false; // ImGui demo window

        const Mesh cube = makeCube();
        const Mesh sphere = makeSphere();

        const Texture white(glm::vec3(1.0f));
        const Texture crateDiffuse(TEXTURE_DIR "container2.png");
        const Texture crateSpecular(TEXTURE_DIR "container2_specular.png");

        struct Object {
            const Mesh* mesh;
            glm::vec3 position;
            float yaw;
            glm::vec3 scale;
            Material material;
        };

        // Gold, from the classic OpenGL material tables (devernay.free.fr/cours/opengl/materials.html).
        // Unlike plastic, a metal's highlight takes the metal's own color.
        const Material gold = {nullptr, nullptr, {0.75164f, 0.60648f, 0.22648f}, {0.628281f, 0.555802f, 0.366065f}, 51.2f * 4.0f}; // table value is for Phong

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

            ImGui::SeparatorText("Light");
            ImGui::Combo("Type", &light.type, "Directional\0Point\0Spot (flashlight)\0");
            ImGui::ColorEdit3("Ambient", glm::value_ptr(light.ambient));
            ImGui::ColorEdit3("Diffuse", glm::value_ptr(light.diffuse));
            ImGui::ColorEdit3("Specular", glm::value_ptr(light.specular));

            if (light.type == Directional)
                ImGui::DragFloat3("Direction", glm::value_ptr(light.direction), 0.01f, -1.0f, 1.0f);

            if (light.type == Point || light.type == Spot) {
                // Range presets fill linear/quadratic; the drags below fine-tune them.
                const char* current = "custom";

                for (const AttenuationPreset& p : kAttenuationPresets) {
                    if (p.linear == light.linear && p.quadratic == light.quadratic)
                        current = p.name;
                }

                if (ImGui::BeginCombo("Range", current)) {
                    for (const AttenuationPreset& p : kAttenuationPresets) {
                        if (ImGui::Selectable(p.name, p.name == current)) {
                            light.linear = p.linear;
                            light.quadratic = p.quadratic;
                        }
                    }

                    ImGui::EndCombo();
                }

                ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 2.0f, "%.4f");
                ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 2.0f, "%.4f");
            }

            if (light.type == Spot) {
                ImGui::SliderFloat("Inner angle", &light.innerAngle, 1.0f, 60.0f, "%.1f deg");
                // Outer can't be smaller than inner: the fade would divide by a negative width and invert the cone.
                ImGui::SliderFloat("Outer angle", &light.outerAngle, light.innerAngle, 60.0f, "%.1f deg");
                light.outerAngle = std::max(light.outerAngle, light.innerAngle);
            }

            if (light.type == Point)
                ImGui::Checkbox("Orbit", &orbitLight);

            ImGui::Checkbox("Blinn-Phong", &blinn);

            ImGui::SeparatorText("Render");
            ImGui::Checkbox("Wireframe", &wireframe);
            ImGui::Checkbox("ImGui demo", &showDemo);
            ImGui::End();

            if (showDemo)
                ImGui::ShowDemoWindow(&showDemo);

            glViewport(0, 0, width, height);
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

            // Dark background so the lighting stands out.
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Camera's vertical field of view, window aspect ratio (so nothing stretches), and near/far clip planes.
            // A Vulkan backend would need a different projection here: depth range 0..1 instead of -1..1, and Y flipped.
            const glm::mat4 projection = glm::perspective(glm::radians(camera.fov), static_cast<float>(width) / height, 0.1f, 100.0f);
            const glm::mat4 view = camera.view();

            // Where the light is and where it points, for the types that need it.
            // Point: circles the scene (radius 2, 1.5 above the floor), so shading changes without moving the camera.
            // Spot: a flashlight in the camera, pointing where you look.
            glm::vec3 lightPos(0.0f);
            glm::vec3 lightDir = light.direction;

            if (light.type == Point) {
                const float angle = orbitLight ? static_cast<float>(now) : 0.0f;
                lightPos = glm::vec3(2.0f * std::cos(angle), 1.5f, 2.0f * std::sin(angle));
            } else if (light.type == Spot) {
                lightPos = camera.position;
                lightDir = camera.front();
            }

            lit.use();
            lit.setMat4("projection", projection);
            lit.setMat4("view", view);
            lit.setVec3("viewPos", camera.position); // specular depends on where the viewer is
            lit.setBool("blinn", blinn);

            // Sent every frame since the panel can change them. Struct fields are separate uniforms, "struct.field".
            lit.setInt("light.type", light.type);
            lit.setVec3("light.position", lightPos);
            lit.setVec3("light.direction", lightDir);
            lit.setVec3("light.ambient", light.ambient);
            lit.setVec3("light.diffuse", light.diffuse);
            lit.setVec3("light.specular", light.specular);
            lit.setFloat("light.constant", light.constant);
            lit.setFloat("light.linear", light.linear);
            lit.setFloat("light.quadratic", light.quadratic);
            // The shader compares cosines (cheaper than angles per pixel), so convert once here.
            lit.setFloat("light.cutOff", std::cos(glm::radians(light.innerAngle)));
            lit.setFloat("light.outerCutOff", std::cos(glm::radians(light.outerAngle)));

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

            // The lamp: a small sphere where a point light is, so you can see where the light comes from.
            // A directional light has no position, and a spot sits inside the camera, so neither gets one.
            if (light.type == Point) {
                glm::mat4 lampModel = glm::translate(glm::mat4(1.0f), lightPos);
                lampModel = glm::scale(lampModel, glm::vec3(0.2f));

                lamp.use();
                lamp.setMat4("projection", projection);
                lamp.setMat4("view", view);
                lamp.setMat4("model", lampModel);
                lamp.setVec3("lightColor", light.diffuse); // the lamp shows the light's main color

                sphere.draw();
            }

            // UI last, on top of the scene. Always filled, even when the scene is drawn as wireframe.
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
