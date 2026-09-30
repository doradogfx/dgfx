#pragma once

#include <glad/glad.h>

class ShadowMap {
public:
    explicit ShadowMap(int resolution);
    ~ShadowMap();

    void resize(int resolution);
    void bind() const;
    void bindDepth(GLuint unit) const;

    GLuint textureId() const { return depth; }

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

private:
    void create();
    void destroy();

    GLuint fbo = 0;
    GLuint depth = 0;
    int resolution = 0;
};
