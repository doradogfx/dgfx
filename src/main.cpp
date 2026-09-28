#include <glad/glad.h> // must come before GLFW so GLFW doesn't pull in the system GL header
#include <GLFW/glfw3.h>

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

    // Each vertex's position arrives through attribute location 0, fed from the vertex buffer below.
    const char* vsSrc = R"(#version 460 core
        layout(location = 0) in vec2 pos;
        void main() { gl_Position = vec4(pos, 0.0, 1.0); }
        )";
    const char* fsSrc = R"(#version 460 core
        out vec4 color;
        void main() { color = vec4(1.0, 0.5, 0.2, 1.0); }
        )";

    // Compile errors don't stop anything on their own; the driver only reports them if asked.
    auto compile = [](GLenum type, const char* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint ok;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader compile error:\n%s\n", log);
        }
        return shader;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Program link error:\n%s\n", log);
    }
    // The linked program keeps its own copy; the shader objects are no longer needed.
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Each corner is stored once...
    const float vertices[] = {
        -0.5f, -0.5f, // 0 bottom-left
         0.5f, -0.5f, // 1 bottom-right
         0.5f,  0.5f, // 2 top-right
        -0.5f,  0.5f, // 3 top-left
    };

    // ...and the triangles refer to corners by index, so the shared diagonal isn't duplicated.
    const GLuint indices[] = {
        0, 1, 2,
        2, 3, 0,
    };

    // The VAO records the attribute layout and which buffer each attribute reads from.
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Copy the vertex data into GPU memory.
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Unlike GL_ARRAY_BUFFER, the element buffer binding is stored in the VAO itself,
    // so it must be bound while the VAO is bound.
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Attribute 0 = 2 floats per vertex, tightly packed, starting at offset 0 of the bound buffer.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glUseProgram(program);

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

        // 6 indices, stored as unsigned ints, read from offset 0 of the VAO's element buffer.
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
