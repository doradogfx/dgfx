#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include "camera.h"
#include "shader.h"
#include "texture.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

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
        Shader shader(SHADER_DIR "basic.vert", SHADER_DIR "basic.frag");

        GLuint container = loadTexture(TEXTURE_DIR "container.jpg");
        GLuint face = loadTexture(TEXTURE_DIR "awesomeface.png");

        // A unit cube centered on the origin. Interleaved: each vertex is x, y, z, then u, v.
        // Corners can't be shared between faces because each face needs its own UVs, so 4 vertices per face.
        // Each face lists bottom-left, bottom-right, top-right, top-left as seen from outside the cube.
        const float vertices[] = {
            // front (+z)
            -0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   0.0f, 1.0f,
            // back (-z)
             0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
            // left (-x)
            -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
            // right (+x)
             0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,   0.0f, 1.0f,
            // top (+y)
            -0.5f,  0.5f,  0.5f,   0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
            // bottom (-y)
            -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,   0.0f, 1.0f,
        };

        // Two triangles per face sharing the diagonal, same pattern as a single quad, offset by 4 vertices per face.
        unsigned int indices[36];

        for (unsigned int f = 0; f < 6; f++) {
            const unsigned int quad[] = { 0, 1, 2, 2, 3, 0 };

            for (int i = 0; i < 6; i++)
                indices[f * 6 + i] = f * 4 + quad[i];
        }

        // Where each cube sits in the world. One mesh, drawn once per position with its own model matrix.
        const glm::vec3 cubePositions[] = {
            { 0.0f,  0.0f,   0.0f},
            { 2.0f,  5.0f, -15.0f},
            {-1.5f, -2.2f,  -2.5f},
            {-3.8f, -2.0f, -12.3f},
            { 2.4f, -0.4f,  -3.5f},
            {-1.7f,  3.0f,  -7.5f},
            { 1.3f, -2.0f,  -2.5f},
            { 1.5f,  2.0f,  -2.5f},
            { 1.5f,  0.2f,  -1.5f},
            {-1.3f,  1.0f,  -1.5f},
        };

        // The VAO records the attribute layout and which buffers the attributes and indices read from.
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Copy the vertex data into GPU memory.
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Copy the indices too. The EBO binding is stored in the VAO, so it must stay bound while the VAO is.
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Stride = bytes from one vertex to the next (5 floats). The offset says where in each vertex the attribute starts.
        // Attribute 0 = position: 3 floats at offset 0.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        // Attribute 1 = texture coordinate: 2 floats right after the position.
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        shader.use();

        // Each fragment shader sampler reads from the texture unit given by its layout binding.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, container);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, face);

        // Draw wireframe
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // Keep, per pixel, only the fragment closest to the camera. Without it, whatever is drawn last wins,
        // so back faces can cover front faces.
        glEnable(GL_DEPTH_TEST);

        Camera camera;

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

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Camera's vertical field of view, window aspect ratio (so nothing stretches), and near/far clip planes.
            // A Vulkan backend would need a different projection here: depth range 0..1 instead of -1..1, and Y flipped.
            const glm::mat4 projection = glm::perspective(glm::radians(camera.fov), static_cast<float>(width) / height, 0.1f, 100.0f);
            shader.setMat4("projection", projection);
            shader.setMat4("view", camera.view());

            const float time = static_cast<float>(now);

            for (int i = 0; i < 10; i++) {
                // Right to left: rotate the cube around its own center, then move it to its place in the world.
                // The index offsets the angle so the cubes don't all spin in sync.
                glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
                model = glm::rotate(model, time + i, glm::vec3(1.0f, 0.3f, 0.5f));
                shader.setMat4("model", model);

                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
            }

            glfwSwapBuffers(window);
        }

        glDeleteTextures(1, &face);
        glDeleteTextures(1, &container);
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
