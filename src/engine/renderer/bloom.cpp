#include "renderer/bloom.h"

#include "core/paths.h"

#include <algorithm>

Bloom::Bloom()
    : down(assetRoot() + "shaders/post.vert", assetRoot() + "shaders/bloom_down.frag"),
      up(assetRoot() + "shaders/post.vert", assetRoot() + "shaders/bloom_up.frag") {
    glGenFramebuffers(1, &fbo);
}

Bloom::~Bloom() {
    destroy();
    glDeleteFramebuffers(1, &fbo);
}

void Bloom::destroy() {
    for (const Level& level : levels)
        glDeleteTextures(1, &level.texture);

    levels.clear();
}

void Bloom::resize(int width, int height) {
    destroy();
    sceneWidth = width;
    sceneHeight = height;

    int w = width;
    int h = height;

    for (int i = 0; i < kLevels; i++) {
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);

        Level level{0, w, h};
        glGenTextures(1, &level.texture);
        glBindTexture(GL_TEXTURE_2D, level.texture);
        // A float format with no alpha, 4 bytes per pixel: HDR values at half the memory of RGBA16F.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        levels.push_back(level);
    }
}

GLuint Bloom::apply(const Framebuffer& scene, int width, int height, float radius) {
    if (width != sceneWidth || height != sceneHeight)
        resize(width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Down: each level reads the one above it, the first one reads the scene.
    down.use();

    for (int i = 0; i < kLevels; i++) {
        const Level& level = levels[i];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, level.texture, 0);
        glViewport(0, 0, level.width, level.height);

        if (i == 0) {
            scene.bindColor(0);
        } else {
            glBindTextureUnit(0, levels[i - 1].texture);
        }

        const int sourceWidth = i == 0 ? width : levels[i - 1].width;
        const int sourceHeight = i == 0 ? height : levels[i - 1].height;
        down.setVec2("sourceTexel", glm::vec2(1.0f / sourceWidth, 1.0f / sourceHeight));
        down.setBool("firstLevel", i == 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // Up: add each level onto the next bigger one, from the smallest.
    up.use();
    up.setFloat("radius", radius);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    for (int i = kLevels - 1; i > 0; i--) {
        const Level& target = levels[i - 1];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
        glViewport(0, 0, target.width, target.height);
        glBindTextureUnit(0, levels[i].texture);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glDisable(GL_BLEND);
    return levels[0].texture;
}
