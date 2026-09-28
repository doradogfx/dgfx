#pragma once

#include <glad/glad.h>

// Load an image file into a new 2D texture (RGBA8, mipmapped). Returns the texture ID, or 0 on failure.
GLuint loadTexture(const char* path);
