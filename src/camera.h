#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

struct Camera {
    glm::vec3 position{0.0f, 0.0f, 4.0f};
    float yaw = -90.0f; // degrees around the world Y axis. 0 looks down +X, so -90 looks down -Z (into the screen)
    float pitch = 0.0f; // degrees up/down

    static constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    // Unit vector the camera looks along: yaw and pitch converted to a direction (spherical coordinates).
    glm::vec3 front() const {
        const float y = glm::radians(yaw);
        const float p = glm::radians(pitch);
        return glm::normalize(glm::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
    }

    // Perpendicular to where we look and to world up, so it always points to the camera's right.
    glm::vec3 right() const {
        return glm::normalize(glm::cross(front(), worldUp));
    }

    // Builds the matrix that moves the world so the camera sits at the origin looking down -Z.
    glm::mat4 view() const {
        return glm::lookAt(position, position + front(), worldUp);
    }

    // Turn by the given angles in degrees. Pitch stops short of straight up/down: at exactly 90 degrees
    // front() would be parallel to worldUp, their cross product is zero, and the view flips.
    void turn(float yawDelta, float pitchDelta) {
        yaw += yawDelta;
        pitch = glm::clamp(pitch + pitchDelta, -89.0f, 89.0f);
    }
};
