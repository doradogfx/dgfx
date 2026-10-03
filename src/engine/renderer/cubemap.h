#pragma once

#include <glad/glad.h>

class Cubemap {
public:
    // Loads right, left, top, bottom, front, back from the directory, as sRGB.
    explicit Cubemap(const char* directory);
    ~Cubemap();

    void bind(GLuint unit) const;

    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

private:
    GLuint id = 0;
};
