#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

#include "shader.h"

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

    // Scoped so the Shader destructor (glDeleteProgram) runs while the GL context still exists.
    {
        Shader shader(SHADER_DIR "basic.vert", SHADER_DIR "basic.frag");

        // Interleaved: each vertex is x, y, z followed by r, g, b.
        const float vertices[] = {
            -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
             0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,
        };

        // The VAO records the attribute layout and which buffer each attribute reads from.
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Copy the vertex data into GPU memory.
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Stride = bytes from one vertex to the next (6 floats). The offset says where in each vertex the attribute starts.
        // Attribute 0 = position: 3 floats at offset 0.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        // Attribute 1 = color: 3 floats right after the position.
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        shader.use();

        // Draw wireframe
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // The main loop: poll OS events, render into the back buffer, present it.
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Poll the key's current state; setting the close flag ends the loop on its next check.
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glfwSwapBuffers(window);
        }

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
