#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Texture {
public:
    // srgb: true for color images (diffuse/albedo), sampled as linear by the GPU.
    // false for data images (specular, normal maps): their values are used as-is.
    Texture(const char* path, bool srgb);
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
