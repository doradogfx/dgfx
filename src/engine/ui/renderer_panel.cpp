#include "ui/panels.h"

#include "renderer/renderer.h"

#include <imgui.h>

#include <string>

void rendererPanel(Renderer& renderer) {
    ImGui::SeparatorText("Rendering");
    ImGui::Checkbox("Blinn-Phong", &renderer.blinn);
    ImGui::Checkbox("Wireframe", &renderer.wireframe);
    ImGui::Checkbox("Face culling", &renderer.faceCulling);
    ImGui::Checkbox("Skybox", &renderer.showSkybox);
    ImGui::Checkbox("Environment reflections", &renderer.reflections);
    ImGui::Checkbox("Fresnel", &renderer.fresnel);

    ImGui::SeparatorText("Post-processing");
    ImGui::Combo("Effect", &renderer.postEffect, "None\0Grayscale\0Invert\0Blur\0Sharpen\0Edge detection\0");
    ImGui::SliderFloat("Gamma", &renderer.gamma, 0.5f, 2.0f, "%.2f");

    ImGui::SeparatorText("Sun shadows");
    ImGui::Checkbox("Enabled", &renderer.shadows);

    const int resolutions[] = {1024, 2048, 4096};
    const std::string current = std::to_string(renderer.shadowResolution);

    if (ImGui::BeginCombo("Resolution", current.c_str())) {
        for (int r : resolutions) {
            if (ImGui::Selectable(std::to_string(r).c_str(), r == renderer.shadowResolution))
                renderer.shadowResolution = r;
        }

        ImGui::EndCombo();
    }

    ImGui::SliderFloat("Bias min", &renderer.shadowBiasMin, 0.0f, 0.01f, "%.4f");
    ImGui::SliderFloat("Bias max", &renderer.shadowBiasMax, 0.0f, 0.05f, "%.4f");
    ImGui::Checkbox("PCF (soft edges)", &renderer.pcf);
    ImGui::Checkbox("Cull front faces", &renderer.shadowCullFront);

    if (ImGui::TreeNode("Shadow map")) {
        // GL textures start at the bottom row, ImGui images at the top, so flip V.
        const float size = 200.0f * uiScale();
        ImGui::Image(static_cast<ImTextureID>(renderer.shadowMapTexture()), ImVec2(size, size), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        ImGui::TreePop();
    }
}
