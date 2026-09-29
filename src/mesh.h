#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    void draw() const;

    // Stop meshes from being copied, as they would share (and double-delete) the same GL buffers
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

// Unit cube centered on the origin (side 1). Flat faces: each face has its own 4 vertices and normal.
Mesh makeCube();

// Sphere centered on the origin, radius 0.5 (same size as the cube). More segments/rings = smoother.
Mesh makeSphere(int segments = 32, int rings = 16);
