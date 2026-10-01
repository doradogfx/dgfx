#include "renderer/framebuffer.h"

#include <cstdio>

Framebuffer::Framebuffer(int width, int height) : width(width), height(height) {
    create();
}

Framebuffer::~Framebuffer() {
    destroy();
}

void Framebuffer::resize(int newWidth, int newHeight) {
    if (newWidth == width && newHeight == height)
        return;

    destroy();
    width = newWidth;
    height = newHeight;
    create();
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void Framebuffer::bindColor(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, color);
}

void Framebuffer::create() {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // sRGB color texture: the scene writes linear values, which in plain 8 bits would leave very few levels
    // for dark tones (visible banding). With GL_FRAMEBUFFER_SRGB on, writes are encoded to sRGB (precision
    // where the eye needs it) and sampling decodes back to linear.
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Clamp so kernel effects sampling past the border repeat the edge instead of wrapping to the other side.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    // Depth is only needed for the depth test, never sampled, so a renderbuffer is enough.
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "Framebuffer %dx%d is incomplete\n", width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy() {
    glDeleteRenderbuffers(1, &depth);
    glDeleteTextures(1, &color);
    glDeleteFramebuffers(1, &fbo);
}
