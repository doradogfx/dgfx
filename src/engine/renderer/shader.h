#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <string>

class Shader {
public:
    // Program ID. 0 when the shader has never built: the pass that uses it draws nothing.
    GLuint id = 0;

    // The compile or link log of the last build that failed, empty when the shader is fine.
    std::string error;

    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    // Builds the program again when a file changed since the last build. On a failure the old program stays.
    // Returns true when it built, so the caller can report the result.
    bool reloadIfChanged();

    // The two file names, e.g. "lit.vert + lit.frag".
    std::string name() const;

    // Use/activate the shader
    void use() const;
    // Set a uniform by name. Writes straight to this program, so it doesn't need to be in use.
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec3(const char* name, const glm::vec3& value) const;
    void setMat4(const char* name, const glm::mat4& value) const;

    // Stop shaders from being copied, as they would share the same OpenGL program
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

private:
    void build(); // compiles and links from the files, and records their write times

    std::string vertPath;
    std::string fragPath;
    std::filesystem::file_time_type vertTime;
    std::filesystem::file_time_type fragTime;
};
