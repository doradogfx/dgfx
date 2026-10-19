#pragma once

#include <glad/glad.h>

class Framebuffer {
public:
    // samples > 1: multisample (MSAA). It can't be sampled, only resolved into a normal framebuffer.
    Framebuffer(int width, int height, int samples = 1);
    ~Framebuffer();

    void resize(int width, int height, int samples = 1);
    void bind() const;
    void bindColor(GLuint unit) const;
    void resolveTo(const Framebuffer& target) const; // averages the samples into target, of the same size

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

private:
    void create();
    void destroy();
    void checkComplete() const;

    GLuint fbo = 0;
    GLuint color = 0;        // a texture, when samples == 1
    GLuint colorSamples = 0; // a renderbuffer, when samples > 1
    GLuint depth = 0;
    int width = 0;
    int height = 0;
    int samples = 1;
};
