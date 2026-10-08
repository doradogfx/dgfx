#include "renderer/shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static std::string readFile(const char* path) {
    std::ifstream file(path);

    if (!file)
        std::fprintf(stderr, "Failed to open shader file: %s\n", path);

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

static GLuint compile(GLenum type, const char* path) {
    std::string src = readFile(path);
    const char* srcPtr = src.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader compile error in %s:\n%s\n", path, log);
    }

    return shader;
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertPath.c_str());
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragPath.c_str());

    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);

    GLint linked;
    glGetProgramiv(id, GL_LINK_STATUS, &linked);

    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(id, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Program link error (%s + %s):\n%s\n", vertPath.c_str(), fragPath.c_str(), log);
    }

    // The linked program keeps its own copy; the shader objects are no longer needed.
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() {
    glDeleteProgram(id);
}

void Shader::use() const {
    glUseProgram(id);
}

void Shader::setBool(const char* name, bool value) const {
    glProgramUniform1i(id, glGetUniformLocation(id, name), value);
}

void Shader::setInt(const char* name, int value) const {
    glProgramUniform1i(id, glGetUniformLocation(id, name), value);
}

void Shader::setFloat(const char* name, float value) const {
    glProgramUniform1f(id, glGetUniformLocation(id, name), value);
}

void Shader::setVec2(const char* name, float x, float y) const {
    glProgramUniform2f(id, glGetUniformLocation(id, name), x, y);
}

void Shader::setVec3(const char* name, float x, float y, float z) const {
    glProgramUniform3f(id, glGetUniformLocation(id, name), x, y, z);
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    setVec3(name, value.x, value.y, value.z);
}

void Shader::setVec4(const char* name, float x, float y, float z, float w) const {
    glProgramUniform4f(id, glGetUniformLocation(id, name), x, y, z, w);
}

void Shader::setMat3(const char* name, const glm::mat3& value) const {
    glProgramUniformMatrix3fv(id, glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(const char* name, const glm::mat4& value) const {
    // GLM and GLSL are both column-major, so no transpose.
    glProgramUniformMatrix4fv(id, glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(value));
}
