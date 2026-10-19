#include "ui/panels.h"

#include "renderer/renderer.h"

#include <imgui.h>

#include <algorithm>
#include <string>

void rendererPanel(Renderer& renderer) {
    ImGui::SeparatorText("GPU time");

    if (ImGui::BeginTable("gpu_time", 2, ImGuiTableFlags_SizingStretchProp)) {
        for (const GpuProfiler::Timing& timing : renderer.profiler.timings()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(timing.name.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%.3f ms", timing.ms);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("Total");
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%.3f ms", renderer.profiler.totalMs());
        ImGui::EndTable();
    }

    // A shader that failed keeps drawing with its last working version. Saving its file builds it again.
    ImGui::SeparatorText("Shaders");

    for (const Shader* shader : renderer.shaders()) {
        if (shader->error.empty()) {
            ImGui::TextDisabled("%s  OK", shader->name().c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.35f, 1.0f), "%s  failed", shader->name().c_str());
            ImGui::TextWrapped("%s", shader->error.c_str());
        }
    }

    ImGui::SeparatorText("Rendering");
    ImGui::Checkbox("Blinn-Phong", &renderer.blinn);
    ImGui::Checkbox("Wireframe", &renderer.wireframe);
    ImGui::Checkbox("Face culling", &renderer.faceCulling);
    ImGui::Checkbox("Skybox", &renderer.showSkybox);
    ImGui::Checkbox("Environment reflections", &renderer.reflections);
    ImGui::Checkbox("Fresnel", &renderer.fresnel);

    // MSAA smooths the edges of shapes, not the inside of textures or highlights.
    auto samplesName = [](int samples) { return samples > 1 ? std::to_string(samples) + "x" : std::string("Off"); };

    if (ImGui::BeginCombo("Anti-aliasing (MSAA)", samplesName(renderer.msaaSamples).c_str())) {
        for (int samples = 1; samples <= std::min(8, renderer.maxSamples); samples *= 2) {
            if (ImGui::Selectable(samplesName(samples).c_str(), renderer.msaaSamples == samples))
                renderer.msaaSamples = samples;
        }

        ImGui::EndCombo();
    }

    // The scene renders in HDR (floats), and this pass brings it to the screen. Not about an HDR monitor.
    ImGui::SeparatorText("Tone mapping and bloom");
    ImGui::Combo("Tone map", &renderer.toneMap, "Clip\0Reinhard\0ACES\0");
    ImGui::SliderFloat("Exposure", &renderer.exposure, 0.1f, 4.0f, "%.2f");
    ImGui::SliderFloat("Gamma", &renderer.gamma, 0.5f, 2.0f, "%.2f");
    ImGui::Checkbox("Bloom", &renderer.bloom);
    ImGui::BeginDisabled(!renderer.bloom);
    ImGui::SliderFloat("Bloom strength", &renderer.bloomStrength, 0.0f, 0.3f, "%.3f");
    ImGui::SliderFloat("Bloom radius", &renderer.bloomRadius, 0.001f, 0.02f, "%.4f");
    ImGui::EndDisabled();
    ImGui::SliderFloat("Lamp brightness", &renderer.lampBrightness, 0.0f, 20.0f, "%.1f");

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

    ImGui::DragFloat("Area radius", &renderer.shadowRadius, 0.5f, 1.0f, 500.0f, "%.1f");
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
