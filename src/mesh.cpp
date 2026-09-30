#include "mesh.h"

#include <cmath>
#include <cstddef>
#include <numbers>
#include <utility>

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
    // Attribute 2 = texture coordinate: 2 floats.
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

Mesh::Mesh(Mesh&& other) noexcept
    : vao(std::exchange(other.vao, 0)),
      vbo(std::exchange(other.vbo, 0)),
      ebo(std::exchange(other.ebo, 0)),
      indexCount(std::exchange(other.indexCount, 0)) {
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}

Mesh makeCube() {
    // The normal points straight out of the face. Corners can't be shared between faces because each face
    // needs its own normal, so 4 vertices per face.
    // Each face lists bottom-left, bottom-right, top-right, top-left as seen from outside the cube,
    // so its triangles are counter-clockwise from outside: front faces for culling.
    std::vector<Vertex> vertices = {
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

    // Every face shows the whole image: its 4 corners, in the same bottom-left, bottom-right, top-right,
    // top-left order as the vertices, map to the image's 4 corners.
    const glm::vec2 corners[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    for (size_t i = 0; i < vertices.size(); i++)
        vertices[i].uv = corners[i % 4];

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
            // The image wraps around like a world map: u goes once around (0 to 1),
            // v runs from the north pole (1, the image's top) down to the south pole (0).
            // The duplicated seam column is what lets the last column have u = 1 while the first has u = 0.
            const glm::vec2 uv(static_cast<float>(s) / segments, 1.0f - static_cast<float>(r) / rings);

            vertices.push_back({dir * 0.5f, dir, uv});
        }
    }

    // Each grid cell, between this ring and the next and this segment and the next, is two triangles.
    std::vector<unsigned int> indices;
    const unsigned int perRing = segments + 1;

    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < segments; s++) {
            const unsigned int a = r * perRing + s; // this ring
            const unsigned int b = a + perRing;     // same segment, next ring down

            // Counter-clockwise seen from outside, so the outside is the front face for culling.
            indices.insert(indices.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    }

    return Mesh(vertices, indices);
}
