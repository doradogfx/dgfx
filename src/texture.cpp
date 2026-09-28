#include "texture.h"

// The implementation of the single-header lib is compiled in exactly one .cpp: this one.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>

GLuint loadTexture(const char* path) {
    // Images are stored top row first, but GL expects UV (0, 0) at the bottom-left.
    stbi_set_flip_vertically_on_load(true);

    // Force 4 channels so the upload format is always RGBA, whatever the file has.
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);

    if (!data) {
        std::fprintf(stderr, "Failed to load texture %s: %s\n", path, stbi_failure_reason());
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Wrap: what UVs outside [0, 1] sample. Filter: how texels are blended when scaled down/up.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Copy the pixels to the GPU, then build the smaller mip levels from them.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    return tex;
}
