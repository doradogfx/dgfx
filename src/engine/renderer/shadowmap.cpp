#include "renderer/shadowmap.h"

#include <cstdio>

ShadowMap::ShadowMap(int resolution) : resolution(resolution) {
    create();
}

ShadowMap::~ShadowMap() {
    destroy();
}

void ShadowMap::resize(int newResolution) {
    if (newResolution == resolution)
        return;

    destroy();
    resolution = newResolution;
    create();
}

void ShadowMap::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, resolution, resolution);
}

void ShadowMap::bindDepth(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, depth);
}

void ShadowMap::create() {
    glGenTextures(1, &depth);
    glBindTexture(GL_TEXTURE_2D, depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Outside the map counts as "nothing blocks the light" (max depth), instead of repeating edge texels
    // and casting phantom shadows beyond the area the light's box covers.
    const float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    // Depth reads back in the red channel only; copy it to green and blue so a preview shows grey, not red.
    const GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
    // No color attachment at all, so tell GL not to draw to or read from one.
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "Shadow map %dx%d is incomplete\n", resolution, resolution);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::destroy() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &depth);
}
