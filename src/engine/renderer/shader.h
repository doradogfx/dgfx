#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>

class Shader {
public:
    // Program ID
    GLuint id = 0;

    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    // Use/activate the shader
    void use() const;
    // Set a uniform by name. Writes straight to this program, so it doesn't need to be in use.
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec3(const char* name, const glm::vec3& value) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setMat3(const char* name, const glm::mat3& value) const;
    void setMat4(const char* name, const glm::mat4& value) const;

    // Stop shaders from being copied, as they would share the same OpenGL program
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
};
