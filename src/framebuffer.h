#pragma once

#include <glad/glad.h>

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    void resize(int width, int height);
    void bind() const;
    void bindColor(GLuint unit) const;

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

private:
    void create();
    void destroy();

    GLuint fbo = 0;
    GLuint color = 0;
    GLuint depth = 0;
    int width = 0;
    int height = 0;
};
