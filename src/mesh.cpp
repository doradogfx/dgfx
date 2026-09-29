#include "mesh.h"

#include <cmath>
#include <cstddef>
#include <numbers>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : indexCount(static_cast<GLsizei>(indices.size())) {
    // The VAO records the attribute layout and which buffers the attributes and indices read from.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Copy the vertex data into GPU memory.
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Copy the indices too. The EBO binding is stored in the VAO, so it must stay bound while the VAO is.
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Stride = bytes from one vertex to the next. offsetof says where in each vertex the attribute starts.
    // Attribute 0 = position: 3 floats.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    // Attribute 1 = normal: 3 floats.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}

Mesh makeCube() {
    // The normal points straight out of the face. Corners can't be shared between faces because each face
    // needs its own normal, so 4 vertices per face.
    // Each face lists bottom-left, bottom-right, top-right, top-left as seen from outside the cube.
    const std::vector<Vertex> vertices = {
        // front (+z)
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        // back (-z)
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        // left (-x)
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
        // right (+x)
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},
        // top (+y)
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},
        // bottom (-y)
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
    };

    // Two triangles per face sharing the diagonal, same pattern as a single quad, offset by 4 vertices per face.
    std::vector<unsigned int> indices;

    for (unsigned int f = 0; f < 6; f++) {
        for (unsigned int i : {0, 1, 2, 2, 3, 0})
            indices.push_back(f * 4 + i);
    }

    return Mesh(vertices, indices);
}

Mesh makeSphere(int segments, int rings) {
    const float pi = std::numbers::pi_v<float>;
    std::vector<Vertex> vertices;

    for (int r = 0; r <= rings; r++) {
        const float theta = pi * r / rings;

        for (int s = 0; s <= segments; s++) {
            const float phi = 2.0f * pi * s / segments;

            // Point on a sphere of radius 1: this is also the normal, the direction straight out from the center.
            const glm::vec3 dir(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
            vertices.push_back({dir * 0.5f, dir});
        }
    }

    // Each grid cell, between this ring and the next and this segment and the next, is two triangles.
    std::vector<unsigned int> indices;
    const unsigned int perRing = segments + 1;

    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < segments; s++) {
            const unsigned int a = r * perRing + s; // this ring
            const unsigned int b = a + perRing;     // same segment, next ring down

            indices.insert(indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }

    return Mesh(vertices, indices);
}
