#include "renderer/cubemap.h"

#include <stb_image.h>

#include <cstdio>
#include <string>

Cubemap::Cubemap(const char* directory) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    // Order fixed by GL: +X, -X, +Y, -Y, +Z, -Z, uploaded to GL_TEXTURE_CUBE_MAP_POSITIVE_X + i.
    const char* faces[] = {"right", "left", "top", "bottom", "front", "back"};

    // Cubemap faces use a top-left origin, unlike 2D textures. stb's flip flag is global and Texture turns
    // it on, so turn it off explicitly or load order would decide whether the sky is upside down.
    stbi_set_flip_vertically_on_load(false);

    for (int i = 0; i < 6; i++) {
        const std::string path = std::string(directory) + faces[i] + ".jpg";
        int width, height, channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

        if (!data) {
            std::fprintf(stderr, "Failed to load cubemap face %s: %s\n", path.c_str(), stbi_failure_reason());
            continue;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

Cubemap::~Cubemap() {
    glDeleteTextures(1, &id);
}

void Cubemap::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
}
