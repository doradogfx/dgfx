#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include "camera.h"
#include "mesh.h"
#include "shader.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

// Plastic: the surface color everywhere, plus a white-ish highlight (the light's color, not the surface's).
static Material plastic(glm::vec3 color, float shininess = 128.0f) {
    return {color, color, glm::vec3(0.5f), shininess};
}

// Rubber: the surface color, almost no highlight, and what little there is is wide and dull.
static Material rubber(glm::vec3 color) {
    return {color, color, glm::vec3(0.1f), 8.0f};
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

        // Light parameters. Each is an intensity per term; the materials decide how much of it each surface reflects.
        const glm::vec3 lightAmbient(0.1f);  // 0 = faces turned away from the light go pitch black
        const glm::vec3 lightDiffuse(1.0f);  // the light's color; try (1, 0.3, 0.3) for a red light
        const glm::vec3 lightSpecular(1.0f); // 0 = no highlights on anything
        const bool blinn = true;             // false = classic Phong, to compare the highlights
        const bool orbitLight = true;        // false = light stays still at its starting position

        // Geometry lives on the GPU until these go out of scope (before the GL context is destroyed).
        const Mesh cube = makeCube();
        const Mesh sphere = makeSphere();

        // The scene: a few meshes, each placed, turned, sized and given a material.
        struct Object {
            const Mesh* mesh;
            glm::vec3 position;
            float yaw; // degrees around Y
            glm::vec3 scale;
            Material material;
        };

        // Gold, from the classic OpenGL material tables (devernay.free.fr/cours/opengl/materials.html).
        // Unlike plastic, a metal's highlight takes the metal's own color.
        const Material gold = {{0.24725f, 0.1995f, 0.0745f}, {0.75164f, 0.60648f, 0.22648f}, {0.628281f, 0.555802f, 0.366065f}, 51.2f * 4.0f}; // table value is for Phong

        const Object objects[] = {
            {&cube,   { 0.0f, -0.55f,  0.0f},  0.0f, {10.0f, 0.1f, 10.0f}, rubber({0.6f, 0.6f, 0.6f})},            // floor, top surface at y = -0.5
            {&sphere, { 0.0f,  0.0f,   0.0f},  0.0f, { 1.0f, 1.0f,  1.0f}, plastic({1.0f, 0.5f, 0.3f})},           // orange
            {&sphere, { 2.0f,  0.0f,  -1.0f}, 30.0f, { 1.0f, 1.0f,  1.0f}, plastic({0.3f, 0.5f, 1.0f}, 512.0f)},   // blue, glossy
            {&sphere, {-1.5f, -0.25f,  1.0f},  0.0f, { 0.5f, 0.5f,  0.5f}, rubber({0.4f, 0.9f, 0.4f})},            // small green
            {&sphere, { 1.5f,  0.0f,   1.0f}, 45.0f, { 1.0f, 1.0f,  1.0f}, plastic({0.7f, 0.05f, 0.87f})},         // purple
            {&sphere, {-2.2f,  0.0f,  -1.2f}, 15.0f, { 1.0f, 1.0f,  1.0f}, gold},                                  // gold
        };

        // Values that never change during the run only need to be sent once.
        // Struct fields are separate uniforms in GL, addressed as "struct.field".
        lit.setVec3("light.ambient", lightAmbient);
        lit.setVec3("light.diffuse", lightDiffuse);
        lit.setVec3("light.specular", lightSpecular);
        lit.setBool("blinn", blinn);
        lamp.setVec3("lightColor", lightDiffuse); // the lamp shows the light's main color

        glEnable(GL_DEPTH_TEST);

        Camera camera;
        camera.position = glm::vec3(0.0f, 2.0f, 6.0f);
        camera.pitch = -15.0f;

        // Hide the cursor and lock it to the window, so the mouse can turn the camera forever without hitting the screen edge.
        // Released when the window loses focus, grabbed again by clicking in the window.
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        bool cursorCaptured = true;

        // Raw motion skips the OS pointer acceleration, so the same hand movement always turns the same amount.
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        // Mouse look works on how far the cursor moved since last frame, so remember where it was.
        double lastX, lastY;
        glfwGetCursorPos(window, &lastX, &lastY);

        // The scroll wheel has no current state to poll, GLFW only reports it through a callback.
        // The callback adds to this, the loop consumes it. The window's user pointer is how the
        // capture-less callback finds it.
        double scroll = 0.0;
        glfwSetWindowUserPointer(window, &scroll);
        glfwSetScrollCallback(window, [](GLFWwindow* w, double, double yOffset) {
            *static_cast<double*>(glfwGetWindowUserPointer(w)) += yOffset;
        });

        double lastTime = glfwGetTime();

        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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

            // Give the cursor back when switching to another window; take it again on a click inside this one.
            if (cursorCaptured && !glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                cursorCaptured = false;
            } else if (!cursorCaptured && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                cursorCaptured = true;
                // Start measuring from here, or the distance moved while free would turn the camera in one jump.
                glfwGetCursorPos(window, &lastX, &lastY);
            }

            // Mouse look: horizontal movement turns left/right (yaw), vertical tilts up/down (pitch).
            // Screen Y grows downward, so moving the mouse up gives a negative dy, which should tilt up: hence the minus.
            const float sensitivity = 0.1f; // degrees per pixel
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            if (cursorCaptured)
                camera.turn(static_cast<float>(x - lastX) * sensitivity, static_cast<float>(lastY - y) * sensitivity);

            lastX = x;
            lastY = y;

            // Scroll up zooms in, 2 degrees of field of view per wheel notch.
            camera.zoom(static_cast<float>(scroll) * 2.0f);
            scroll = 0.0;

            // WASD moves along where the camera looks (flying, so looking up and pressing W goes up).
            // Space/Left Ctrl move straight up/down in the world. Holding Left Shift moves 4x faster.
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

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            // Minimized: the framebuffer is 0x0, so there's nothing to draw and the aspect ratio would divide by zero.
            // Sleep until the next event instead of spinning through empty frames.
            if (width == 0 || height == 0) {
                glfwWaitEvents();
                continue;
            }

            glViewport(0, 0, width, height);

            // Dark background so the lighting stands out.
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Camera's vertical field of view, window aspect ratio (so nothing stretches), and near/far clip planes.
            // A Vulkan backend would need a different projection here: depth range 0..1 instead of -1..1, and Y flipped.
            const glm::mat4 projection = glm::perspective(glm::radians(camera.fov), static_cast<float>(width) / height, 0.1f, 100.0f);
            const glm::mat4 view = camera.view();

            // The light circles the scene (radius 2, 1.5 above the floor), so shading changes without moving the camera.
            const float angle = orbitLight ? static_cast<float>(now) : 0.0f;
            const glm::vec3 lightPos(2.0f * std::cos(angle), 1.5f, 2.0f * std::sin(angle));

            lit.use();
            lit.setMat4("projection", projection);
            lit.setMat4("view", view);
            lit.setVec3("light.position", lightPos);
            lit.setVec3("viewPos", camera.position); // specular depends on where the viewer is

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
                lit.setVec3("material.ambient", obj.material.ambient);
                lit.setVec3("material.diffuse", obj.material.diffuse);
                lit.setVec3("material.specular", obj.material.specular);
                lit.setFloat("material.shininess", obj.material.shininess);

                obj.mesh->draw();
            }

            // The lamp: a small sphere where the light is, so you can see where the light comes from.
            glm::mat4 lampModel = glm::translate(glm::mat4(1.0f), lightPos);
            lampModel = glm::scale(lampModel, glm::vec3(0.2f));

            lamp.use();
            lamp.setMat4("projection", projection);
            lamp.setMat4("view", view);
            lamp.setMat4("model", lampModel);

            sphere.draw();

            glfwSwapBuffers(window);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
