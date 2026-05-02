#include "ColorGrading.h"

#include "Shader.h"
#include "Graphics.h"
#include "Scene.h"

#include <imgui.h>

ColorGrading::ColorGrading() {
    this->colorGradingShader = new ComputeShaderDispatch(
        new ComputeShaderProgram("./res/shaders/color_grading.comp")
    );
}

void ColorGrading::OnPostProcess(const PostProcessParams* params) {
    if (this->colorGradingShader) {
        auto* data = this->colorGradingShader->GetData();

        data->SetValue("inputTex", params->inputTexture);
        data->SetValue("outputImg", params->outputTexture);

        data->SetValue("brightness", this->brightness);
        data->SetValue("contrast", this->contrast);
        data->SetValue("saturation", this->saturation);

        if (this->curveTexture != nullptr) {
            data->SetValue("useCurve", 1.0f); // wtf
            data->SetValue("curveTex", this->curveTexture);
        } else {
            data->SetValue("useCurve", 0.0f); // wtf
        }

        data->SetValue("chromaticAberrationStrength", this->chromaticAberrationStrength);
        data->SetValue("chromaticAberrationOffsets", this->chromaticAberrationOffsets);

        data->SetValue("filmGrainStrength", this->filmGrainStrength);
        data->SetValue("vignetteStrength", this->vignetteStrength);

    }

    glm::vec2 res = GetScene()->GetGraphics()->GetScreenResolution();
    this->colorGradingShader->Dispatch(std::ceil(res.x / 8.0f), std::ceil(res.y / 8.0f), 1);
}

void ColorGrading::SetCurveTexture(Texture2D* texture) {
    this->curveTexture = texture;
};

void ColorGrading::DrawImGui() {
    ImGui::Text("Color Grading");
    ImGui::Spacing();

    ImGui::SliderFloat("Brightness", &this->brightness, 0.0f, 3.0f);
    ImGui::SliderFloat("Contrast", &this->contrast, 0.0f, 3.0f);
    ImGui::SliderFloat("Saturation", &this->saturation, 0.0f, 3.0f);
    ImGui::Separator();
    ImGui::TextUnformatted("Chromatic Aberration");
    ImGui::InputFloat3("Offsets", &this->chromaticAberrationOffsets.x);
    ImGui::SliderFloat("Strength", &this->chromaticAberrationStrength, 0.0f, 5.0f);
    ImGui::Separator();
    ImGui::SliderFloat("Film Grain", &this->filmGrainStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Vignette", &this->vignetteStrength, 0.0f, 1.0f);

    if (ImGui::Button("Reset")) {
        this->brightness = 1.0f;
        this->contrast = 1.0f;
        this->saturation = 1.0f;
        this->chromaticAberrationStrength = 0.0f;
        this->chromaticAberrationOffsets = { 0.009f, 0.006f, -0.006f };
        this->filmGrainStrength = 0.0f;
        this->vignetteStrength = 0.0f;
    }

    if (this->curveTexture != nullptr) {
        ImGui::Text("Curve Texture Enabled");
        if (ImGui::Button("Remove Curve Texture")) {
            this->curveTexture = nullptr;
        }
    } else {
        ImGui::Text("Curve Texture Disabled");
    }
}
