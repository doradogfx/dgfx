#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>

class Texture {
public:
    // srgb: true for color images (diffuse/albedo), sampled as linear by the GPU.
    // false for data images (specular, normal maps): their values are used as-is.
    Texture(const char* path, bool srgb);
    explicit Texture(glm::vec3 color);
    ~Texture();

    // Bind to a texture unit, where a sampler with layout(binding = unit) reads it.
    void bind(GLuint unit) const;

    bool isSrgb = false; // loaded as a color image, needed to save a material that uses it

    // Stop textures from being copied, as they would share (and double-delete) the same GL texture
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

private:
    GLuint id = 0;
};

// Loads each image file once and hands out the same Texture to everyone who asks for it.
class TextureCache {
public:
    const Texture& get(const std::string& path, bool srgb);

    // The path a texture was loaded from, or empty if the cache doesn't own it.
    std::string pathOf(const Texture& texture) const;

private:
    // unique_ptr keeps every Texture at a fixed address as the map grows, so materials can hold plain pointers.
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
};
