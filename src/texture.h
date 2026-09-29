#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Texture {
public:
    explicit Texture(const char* path);
    explicit Texture(glm::vec3 color);
    ~Texture();

    // Bind to a texture unit, where a sampler with layout(binding = unit) reads it.
    void bind(GLuint unit) const;

    // Stop textures from being copied, as they would share (and double-delete) the same GL texture
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

private:
    GLuint id = 0;
};
