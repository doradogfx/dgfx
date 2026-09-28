#pragma once

#include <glad/glad.h>

class Shader {
public:
    // Program ID
    GLuint id = 0;

    Shader(const char* vertPath, const char* fragPath);
    ~Shader();

    // Use/activate the shader
    void use() const;
    // Set a uniform by name. Writes straight to this program, so it doesn't need to be in use.
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;

    // Stop shaders from being copied, as they would share the same OpenGL program
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
};
