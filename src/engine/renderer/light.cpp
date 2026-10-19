#include "renderer/light.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>

void setSun(const Shader& shader, const WorldDirLight& dir) {
    shader.setBool("dirLight.enabled", dir.light.enabled);
    shader.setVec3("dirLight.direction", dir.direction);
    shader.setVec3("dirLight.ambient", dir.light.ambient);
    shader.setVec3("dirLight.diffuse", dir.light.diffuse);
    shader.setVec3("dirLight.specular", dir.light.specular);
}

GpuLight toGpu(const WorldPointLight& point) {
    const PointLight& l = point.light;
    // cos is never below -1, so the cone fade is always 1: light in every direction.
    return {point.position, l.constant, glm::vec3(0.0f, 0.0f, -1.0f), l.linear, l.ambient, l.quadratic,
            l.diffuse, -1.0f, l.specular, -2.0f};
}

GpuLight toGpu(const WorldSpotLight& spot) {
    const SpotLight& l = spot.light;
    // The shader compares cosines, so convert once here instead of per pixel.
    return {spot.position, l.constant, spot.direction, l.linear, l.ambient, l.quadratic,
            l.diffuse, std::cos(glm::radians(l.innerAngle)), l.specular, std::cos(glm::radians(l.outerAngle))};
}

static void colorsUI(glm::vec3& ambient, glm::vec3& diffuse, glm::vec3& specular) {
    // HDR: the scene renders in floats, so a light can be brighter than white (values above 1).
    const ImGuiColorEditFlags flags = ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float;
    ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient), flags);
    ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse), flags);
    ImGui::ColorEdit3("Specular", glm::value_ptr(specular), flags);
}

static void attenuationUI(float& linear, float& quadratic) {
    const char* current = "custom";

    for (const AttenuationPreset& p : kAttenuationPresets) {
        if (p.linear == linear && p.quadratic == quadratic)
            current = p.name;
    }

    if (ImGui::BeginCombo("Range", current)) {
        for (const AttenuationPreset& p : kAttenuationPresets) {
            if (ImGui::Selectable(p.name, p.name == current)) {
                linear = p.linear;
                quadratic = p.quadratic;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::DragFloat("Linear", &linear, 0.001f, 0.0f, 2.0f, "%.4f");
    ImGui::DragFloat("Quadratic", &quadratic, 0.001f, 0.0f, 2.0f, "%.4f");
}

void lightUI(DirLight& light) {
    ImGui::Checkbox("Enabled", &light.enabled);
    colorsUI(light.ambient, light.diffuse, light.specular);
}

void lightUI(PointLight& light) {
    ImGui::Checkbox("Enabled", &light.enabled);
    colorsUI(light.ambient, light.diffuse, light.specular);
    attenuationUI(light.linear, light.quadratic);
}

void lightUI(SpotLight& light) {
    ImGui::Checkbox("Enabled", &light.enabled);
    colorsUI(light.ambient, light.diffuse, light.specular);
    attenuationUI(light.linear, light.quadratic);
    ImGui::SliderFloat("Inner angle", &light.innerAngle, 1.0f, 60.0f, "%.1f deg");
    // Outer below inner would make the fade divide by a negative width and invert the cone.
    ImGui::SliderFloat("Outer angle", &light.outerAngle, light.innerAngle, 60.0f, "%.1f deg");
    light.outerAngle = std::max(light.outerAngle, light.innerAngle);
}
