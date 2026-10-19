#include "renderer/framebuffer.h"

#include <cstdio>

Framebuffer::Framebuffer(int width, int height, int samples) : width(width), height(height), samples(samples) {
    create();
}

Framebuffer::~Framebuffer() {
    destroy();
}

void Framebuffer::resize(int newWidth, int newHeight, int newSamples) {
    if (newWidth == width && newHeight == height && newSamples == samples)
        return;

    destroy();
    width = newWidth;
    height = newHeight;
    samples = newSamples;
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

// ponytail: the blit averages HDR values before the tone map, so one very bright sample (the sun disk, a lamp)
// can still make a hard edge. If it shows, resolve in a shader that tone maps each sample first.
void Framebuffer::resolveTo(const Framebuffer& target) const {
    glBlitNamedFramebuffer(fbo, target.fbo, 0, 0, width, height, 0, 0, target.width, target.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Framebuffer::create() {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    if (samples > 1) {
        // Multisample: color and depth keep `samples` values per pixel, so edges average to smooth steps.
        glGenRenderbuffers(1, &colorSamples);
        glBindRenderbuffer(GL_RENDERBUFFER, colorSamples);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA16F, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorSamples);

        glGenRenderbuffers(1, &depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        checkComplete();
        return;
    }

    // Half-float color (HDR): values above 1.0 survive, so bright lights stay brighter than white until the post
    // pass tone maps them. Floats also keep enough precision in the dark tones, where 8 bits would band.
    // GL_FRAMEBUFFER_SRGB does nothing to a float target: it stores the linear values as they are.
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Clamp so filters sampling past the border (bloom) repeat the edge instead of wrapping to the other side.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    // Depth is only needed for the depth test, never sampled, so a renderbuffer is enough.
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    checkComplete();
}

void Framebuffer::checkComplete() const {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "Framebuffer %dx%d (%d samples) is incomplete\n", width, height, samples);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy() {
    glDeleteRenderbuffers(1, &depth);
    glDeleteRenderbuffers(1, &colorSamples);
    glDeleteTextures(1, &color);
    glDeleteFramebuffers(1, &fbo);
}
