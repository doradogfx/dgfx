#include "game/terrain.h"

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#include <algorithm>
#include <vector>

float heightAt(const Terrain& terrain, float x, float z) {
    if (terrain.amplitude == 0.0f)
        return terrain.base;

    // The seed selects a different slice of 3D noise.
    const float noise = stb_perlin_fbm_noise3(x * terrain.frequency, static_cast<float>(terrain.seed) + 0.5f,
                                              z * terrain.frequency, 2.0f, 0.5f, terrain.octaves);
    return terrain.base + terrain.amplitude * noise;
}

Mesh makeTerrainMesh(const Terrain& terrain) {
    const int n = std::max(terrain.resolution, 1);
    const float step = terrain.size / static_cast<float>(n);
    const float start = -0.5f * terrain.size;

    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<size_t>((n + 1) * (n + 1)));

    // One vertex at each grid point. The normal comes from the slope between the two neighbors on each axis.
    for (int j = 0; j <= n; j++) {
        for (int i = 0; i <= n; i++) {
            const float x = start + static_cast<float>(i) * step;
            const float z = start + static_cast<float>(j) * step;
            const glm::vec3 normal = glm::normalize(glm::vec3(heightAt(terrain, x - step, z) - heightAt(terrain, x + step, z),
                                                              2.0f * step,
                                                              heightAt(terrain, x, z - step) - heightAt(terrain, x, z + step)));
            const glm::vec2 uv(static_cast<float>(i) / static_cast<float>(n), static_cast<float>(j) / static_cast<float>(n));
            vertices.push_back({{x, heightAt(terrain, x, z), z}, normal, uv});
        }
    }

    // Two triangles for each quad, counter-clockwise when seen from above.
    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(n * n * 6));

    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            const unsigned int a = static_cast<unsigned int>(j * (n + 1) + i);
            const unsigned int b = a + 1;
            const unsigned int c = a + static_cast<unsigned int>(n + 1);
            const unsigned int d = c + 1;
            indices.insert(indices.end(), {a, c, b, b, c, d});
        }
    }

    return Mesh(vertices, indices);
}
