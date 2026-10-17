#include "renderer/shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <system_error>

// The file's text, or an empty string with `error` set.
static std::string readFile(const std::string& path, std::string& error) {
    std::ifstream file(path);

    if (!file) {
        error = "Can't open " + path;
        return {};
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// A compiled shader object, or 0 with the log in `error`.
static GLuint compile(GLenum type, const std::string& path, std::string& error) {
    const std::string src = readFile(path, error);

    if (!error.empty())
        return 0;

    const char* srcPtr = src.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        error = "Compile error in " + path + ":\n" + log;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// The file's write time. A missing file gives the minimum time, so it counts as changed when it comes back.
static std::filesystem::file_time_type writeTime(const std::string& path) {
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(path, ec);
    return ec ? std::filesystem::file_time_type::min() : time;
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath) : vertPath(vertPath), fragPath(fragPath) {
    build();
}

Shader::~Shader() {
    glDeleteProgram(id);
}

void Shader::build() {
    // Before the compile: a save during the compile then counts as a new change.
    vertTime = writeTime(vertPath);
    fragTime = writeTime(fragPath);

    std::string newError;
    const GLuint vs = compile(GL_VERTEX_SHADER, vertPath, newError);
    const GLuint fs = vs ? compile(GL_FRAGMENT_SHADER, fragPath, newError) : 0;
    GLuint program = 0;

    if (vs && fs) {
        program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);

        if (!linked) {
            char log[1024];
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            newError = "Link error (" + name() + "):\n" + log;
            glDeleteProgram(program);
            program = 0;
        }
    }

    // The linked program keeps its own copy; the shader objects are no longer needed. 0 is ignored.
    glDeleteShader(vs);
    glDeleteShader(fs);

    error = newError;

    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return; // keep the old program, if there is one
    }

    glDeleteProgram(id);
    id = program;
}

bool Shader::reloadIfChanged() {
    if (writeTime(vertPath) == vertTime && writeTime(fragPath) == fragTime)
        return false;

    build();
    return true;
}

std::string Shader::name() const {
    return std::filesystem::path(vertPath).filename().string() + " + " + std::filesystem::path(fragPath).filename().string();
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

void Shader::setVec2(const char* name, const glm::vec2& value) const {
    glProgramUniform2fv(id, glGetUniformLocation(id, name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    glProgramUniform3fv(id, glGetUniformLocation(id, name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const char* name, const glm::mat4& value) const {
    // GLM and GLSL are both column-major, so no transpose.
    glProgramUniformMatrix4fv(id, glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(value));
}
