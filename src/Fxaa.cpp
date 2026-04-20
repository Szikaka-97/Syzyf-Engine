#include <Fxaa.h>

#include <Graphics.h>
#include <Resources.h>
#include <Scene.h>
#include <Shader.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

Fxaa::Fxaa()
{
    auto* shader = GetScene()->Resources()->Get<ComputeShader>("./res/shaders/fxaa/fxaa.comp");

    if (!shader)
    {
        spdlog::error("Failed to load FXAA shader: ./res/shaders/fxaa/fxaa.comp");
        this->fxaaShader = nullptr;
        return;
    }

    this->fxaaShader = new ComputeShaderDispatch(shader);
}

void Fxaa::OnPostProcess(const PostProcessParams* params)
{
    if (!this->fxaaShader || !params)
        return;

    auto* data = this->fxaaShader->GetData();

    data->SetValue("inputTex", params->inputTexture);
    data->SetValue("outputImg", params->outputTexture);

    data->SetValue("debugEdges", this->debugEdges ? 1.0f : 0.0f);

    data->SetValue("edgeThreshold", this->edgeThreshold);
    data->SetValue("edgeThresholdMin", this->edgeThresholdMin);
    data->SetValue("subpixCap", this->subpixCap);
    data->SetValue("subpixTrim", this->subpixTrim);
    data->SetValue("searchThreshold", this->searchThreshold);
    data->SetValue("searchSteps", this->searchSteps);

    glm::vec2 res = GetScene()->GetGraphics()->GetScreenResolution();
    this->fxaaShader->Dispatch(
        static_cast<int>(std::ceil(res.x / 8.0f)),
        static_cast<int>(std::ceil(res.y / 8.0f)),
        1
    );
}

void Fxaa::DrawImGui()
{
    ImGui::Text("FXAA");
    ImGui::Spacing();

    ImGui::Checkbox("debugEdges##FXAA", &this->debugEdges);

    ImGui::Separator();

    const char* presets[] = {
        "FXAA_PRESET 3",
        "FXAA_PRESET 4",
        "FXAA_PRESET 5"
    };
    static int selectedPreset = 2;

    ImGui::Combo("preset##FXAA", &selectedPreset, presets, IM_ARRAYSIZE(presets));

    if (ImGui::Button("Apply preset##FXAA"))
    {
        switch (selectedPreset)
        {
        case 0:
            this->edgeThreshold = 1.0f / 8.0f;
            this->edgeThresholdMin = 1.0f / 16.0f;
            this->searchSteps = 16.0f;
            this->searchThreshold = 1.0f / 4.0f;
            this->subpixCap = 3.0f / 4.0f;
            this->subpixTrim = 1.0f / 4.0f;

            break;

        case 1:
            this->edgeThreshold = 1.0f / 8.0f;
            this->edgeThresholdMin = 1.0f / 24.0f;
            this->searchSteps = 24.0f;
            this->searchThreshold = 1.0f / 4.0f;
            this->subpixCap = 3.0f / 4.0f;
            this->subpixTrim = 1.0f / 4.0f;
            break;

        case 2:
            this->edgeThreshold = 1.0f / 8.0f;
            this->edgeThresholdMin = 1.0f / 24.0f;
            this->searchSteps = 32.0f;
            this->searchThreshold = 1.0f / 4.0f;
            this->subpixCap = 3.0f / 4.0f;
            this->subpixTrim = 1.0f / 4.0f;
            break;
        }
    }

    ImGui::Separator();

    ImGui::SliderFloat("edgeThreshold##FXAA", &this->edgeThreshold, 0.0f, 0.25f, "%.5f");
    ImGui::SliderFloat("edgeThresholdMin##FXAA", &this->edgeThresholdMin, 0.0f, 0.10f, "%.5f");
    ImGui::SliderFloat("subpixCap##FXAA", &this->subpixCap, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("subpixTrim##FXAA", &this->subpixTrim, 0.0f, 0.5f, "%.3f");
    ImGui::SliderFloat("searchThreshold##FXAA", &this->searchThreshold, 0.125f, 0.5f, "%.5f");

    int steps = static_cast<int>(this->searchSteps);
    if (ImGui::SliderInt("searchSteps##FXAA", &steps, 4, 64))
    {
        this->searchSteps = static_cast<float>(steps);
    }

    if (ImGui::Button("Reset FXAA##FXAA"))
    {
        this->debugEdges = false;

        this->edgeThreshold = 1.0f / 8.0f;
        this->edgeThresholdMin = 1.0f / 24.0f;
        this->searchSteps = 32.0f;
        this->searchThreshold = 1.0f / 4.0f;
        this->subpixCap = 3.0f / 4.0f;
        this->subpixTrim = 1.0f / 4.0f;
    }
}
