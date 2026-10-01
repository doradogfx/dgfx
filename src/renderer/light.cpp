#include "renderer/light.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

void setLights(const Shader& shader, const WorldDirLight& dir, std::span<const WorldPointLight> points, const WorldSpotLight& spot) {
    shader.setBool("dirLight.enabled", dir.light.enabled);
    shader.setVec3("dirLight.direction", dir.direction);
    shader.setVec3("dirLight.ambient", dir.light.ambient);
    shader.setVec3("dirLight.diffuse", dir.light.diffuse);
    shader.setVec3("dirLight.specular", dir.light.specular);

    int count = 0;

    for (const WorldPointLight& p : points) {
        if (!p.light.enabled || count == kMaxPointLights)
            continue;

        const std::string prefix = "pointLights[" + std::to_string(count++) + "].";
        auto name = [&](const char* field) { return prefix + field; };

        shader.setVec3(name("position").c_str(), p.position);
        shader.setVec3(name("ambient").c_str(), p.light.ambient);
        shader.setVec3(name("diffuse").c_str(), p.light.diffuse);
        shader.setVec3(name("specular").c_str(), p.light.specular);
        shader.setFloat(name("constant").c_str(), p.light.constant);
        shader.setFloat(name("linear").c_str(), p.light.linear);
        shader.setFloat(name("quadratic").c_str(), p.light.quadratic);
    }

    shader.setInt("numPointLights", count);

    shader.setBool("spotLight.enabled", spot.light.enabled);
    shader.setVec3("spotLight.position", spot.position);
    shader.setVec3("spotLight.direction", spot.direction);
    shader.setVec3("spotLight.ambient", spot.light.ambient);
    shader.setVec3("spotLight.diffuse", spot.light.diffuse);
    shader.setVec3("spotLight.specular", spot.light.specular);
    shader.setFloat("spotLight.constant", spot.light.constant);
    shader.setFloat("spotLight.linear", spot.light.linear);
    shader.setFloat("spotLight.quadratic", spot.light.quadratic);
    // The shader compares cosines, so convert once here instead of per pixel.
    shader.setFloat("spotLight.cutOff", std::cos(glm::radians(spot.light.innerAngle)));
    shader.setFloat("spotLight.outerCutOff", std::cos(glm::radians(spot.light.outerAngle)));
}

static void colorsUI(glm::vec3& ambient, glm::vec3& diffuse, glm::vec3& specular) {
    ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient));
    ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse));
    ImGui::ColorEdit3("Specular", glm::value_ptr(specular));
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
