#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>

static GLuint upload(int width, int height, const unsigned char* pixels, GLenum internalFormat) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Sharper, non-flickering textures on surfaces seen at grazing angles. Core since GL 4.6.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    return tex;
}

Texture::Texture(const char* path, bool srgb) {
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);

    if (!data) {
        std::fprintf(stderr, "Failed to load texture %s: %s\n", path, stbi_failure_reason());
        return;
    }

    id = upload(width, height, data, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8);

    stbi_image_free(data);
}

Texture::Texture(glm::vec3 color) {
    const glm::vec3 c = glm::clamp(color, 0.0f, 1.0f) * 255.0f;

    const unsigned char pixel[4] = {
        static_cast<unsigned char>(c.r), static_cast<unsigned char>(c.g), static_cast<unsigned char>(c.b), 255,
    };

    id = upload(1, 1, pixel, GL_RGBA8);
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}

void Texture::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

const Texture& TextureCache::get(const std::string& path, bool srgb) {
    auto it = textures.find(path);

    if (it == textures.end())
        it = textures.emplace(path, std::make_unique<Texture>(path.c_str(), srgb)).first;

    return *it->second;
}
