#pragma once

#include <Camera.h>
#include <ColorGrading.h>
#include <Fxaa.h>
#include <Light.h>
#include <LightSystem.h>
#include <Scene.h>
#include <Shader.h>
#include <Tonemapper.h>
#include <fog/FogVolume.h>
#include <physics/System.h>
#include <text/Font.h>
#include <text/Text3D.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/UiCheckbox.h>
#include <ui/widgets/UiCircularBar.h>

namespace ExampleUi {
inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<UiSystem>();

#pragma region Ui
    // UiNode
    SceneNode* uiNode = mainScene.CreateNode("Ui Node");
    uiNode->AddObject<UiLayout>(glm::uvec2(200, 200), glm::uvec2(0, 0), 0,
                                AnchorPoint::BottomCenter);
    auto* uiVisual = uiNode->AddObject<UiVisual>(
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        mainScene.Resources()->Get<Texture2D>(
            "./res/textures/1147437805040054272.png",
            Texture2D::ColorTextureRGBA));
    uiVisual->colorHovered = {1.0f, 0.0f, 0.0f, 1.0f};
    uiVisual->colorClicked = {0.0f, 1.0f, 0.0f, 1.0f};
    uiNode->AddObject<UiInteractable>();

    // Child UiNode
    SceneNode* childUiNode = mainScene.CreateNode(uiNode, "Child Ui Node");
    childUiNode->AddObject<UiLayout>(glm::uvec2(80), glm::uvec2(0, 0), 1,
                                     AnchorPoint::BottomRight);
    childUiNode->AddObject<UiInteractable>();
    auto* childUiVisual = childUiNode->AddObject<UiVisual>(
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        mainScene.Resources()->Get<Texture2D>(
            "./res/textures/1147437805040054272.png",
            Texture2D::ColorTextureRGBA));
    childUiVisual->colorHovered = {0.0f, 0.0f, 1.0f, 1.0f};
    childUiVisual->colorClicked = {0.2f, 0.5f, 0.7f, 1.0f};

    // Custom Material Ui Node
    ShaderProgram* customUiProgram =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/ui/ui.vert")
            .WithPixelShader("./res/shaders/ui/custom/circular_bar.frag")
            .Link();
    Material* customUiMaterial = new Material(customUiProgram);
    customUiMaterial->SetValue("highColor", glm::vec4(0.2f, 0.9f, 0.2f, 1.0f));
    customUiMaterial->SetValue("lowColor", glm::vec4(0.9f, 0.1f, 0.1f, 1.0f));
    customUiMaterial->SetValue("backgroundColor",
                               glm::vec4(0.1f, 0.1f, 0.1f, 0.6f));
    customUiMaterial->SetValue("level", 1.0f);

    SceneNode* customUiNode = mainScene.CreateNode("Custom Ui Node");
    customUiNode->AddObject<UiLayout>(glm::uvec2(150), glm::uvec2(-50), 2,
                                      AnchorPoint::BottomRight);
    auto* uiCustomVisual =
        customUiNode->AddObject<UiVisual>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    uiCustomVisual->customMaterial = customUiMaterial;
    customUiNode->AddObject<UiInteractable>();
    auto* circularBar = customUiNode->AddObject<UiCircularBar>();
    circularBar->material = customUiMaterial;

    // UiText
    // To use .ttf fonts here, first they need to be converted using
    // msdf-atlas-gen github.com/Chlumsky/msdf-atlas-gen
    // using this command:
    // msdf-atlas-gen -font
    // msdf-atlas-gen ./OpenSans-Regular.ttf -type
    // msdf -format png -imageout ./OpenSans-Regular.png -json
    // ./OpenSans-Regular.json -size 32 -pxrange 4
    TextureParams fontTextureParams = {.channels = TextureChannels::RGB,
                                       .colorSpace = TextureColor::Linear,
                                       .format = TextureFormat::Ubyte,
                                       .wrapU = TextureWrap::Clamp,
                                       .wrapV = TextureWrap::Clamp,
                                       .minFilter = TextureFilter::Linear,
                                       .magFilter = TextureFilter::Linear};

    Texture2D* openSansFontAtlasTexture = mainScene.Resources()->Get<Texture2D>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.png", fontTextureParams);
    Font* openSansRegularFont = mainScene.Resources()->Get<Font>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.json",
        openSansFontAtlasTexture);
    // Texture2D* jetBrainsMonoFontAtlasTexture =
    //     mainScene.Resources()->Get<Texture2D>(
    //         "./res/fonts/JetBrainsMono-Regular/JetBrainsMono-Regular.png",
    //         fontTextureParams);
    // Font* jetBrainsMonoFont = mainScene.Resources()->Get<Font>(
    //     "./res/fonts/JetBrainsMono-Regular/JetBrainsMono-Regular.json",
    //     jetBrainsMonoFontAtlasTexture);

    SceneNode* uiTextRootNode = mainScene.CreateNode("UiTextRoot");
    uiTextRootNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    uiTextRootNode->AddObject<UiLayout>(glm::ivec2(140.0f, 100.0f),
                                        glm::ivec2(20.0f));

    SceneNode* uiTextNode = mainScene.CreateNode(uiTextRootNode, "UiText");
    uiTextNode->AddObject<UiLayout>(glm::ivec2(100.0f), glm::ivec2(2.0f, -5.0f),
                                    1);
    auto* uiText =
        uiTextNode->AddObject<UiText>("Pooga\nSchnoz", openSansRegularFont);
    uiText->fontSize = 40.0f;
    uiText->color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);

    // 3D Text and Pixelart fonts
    // To use pixelart fonts the command above must be run with "-type hardmask"
    // instead of msdf, a flag must be set to false when loading the font and
    // the texture filter must be set to nearest as well
    TextureParams texParamsNearest = fontTextureParams;
    texParamsNearest.magFilter = TextureFilter::Nearest;
    texParamsNearest.minFilter = TextureFilter::Nearest;

    Texture2D* dougenzakaAtlasTexture = mainScene.Resources()->Get<Texture2D>(
        "./res/fonts/khdotfont-20150527/KH-Dot-Dougenzaka-16.png",
        texParamsNearest);
    Font* dougenzakaFont = mainScene.Resources()->Get<Font>(
        "./res/fonts/khdotfont-20150527/KH-Dot-Dougenzaka-16.json",
        dougenzakaAtlasTexture, false);

    SceneNode* text3dNode = mainScene.CreateNode("Text 3D");
    text3dNode->LocalTransform().Position() = {0.0f, 0.0f, 3.0f};
    text3dNode->GlobalTransform().Scale() = glm::vec3(0.2f);
    auto* textObject = text3dNode->AddObject<Text3D>(
        "Lvl 2 Niebieski Glodny Alfa Krasnolud", dougenzakaFont);
    textObject->color = {1.2f, 0.0f, 0.0f, 1.0f};
    textObject->billboardMode = BillboardMode::Z;

    // Checkbox
    SceneNode* checkboxNode = UiCheckbox::Create(mainScene, dougenzakaFont, 0,
                                                 "Checkbox Example", false);
    if (auto* layout = checkboxNode->GetObject<UiLayout>()) {
        layout->offset = glm::ivec2(100, 200);
    }
    if (auto* checkboxLogic = checkboxNode->GetObject<UiCheckbox>()) {
        checkboxLogic->OnValueChanged = [](bool isChecked) {
            if (isChecked) {
                spdlog::info("Checkbox ON");
            } else {
                spdlog::info("Checkbox OFF");
            }
        };
    }

#pragma endregion

#pragma region Camera

    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    cameraNode->AddObject<ColorGrading>();
    cameraNode->AddObject<Fxaa>();

#pragma endregion
#pragma region Miscellaneous
    SceneNode* sun = mainScene.CreateNode("Sun");
    sun->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 2))
        ->SetShadowCasting(true);
    sun->GlobalTransform().Position() = {1, 2.2f, 0};
    sun->GlobalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(50.0f, -20.0f, 0.0f)));
    mainScene.GetComponent<LightSystem>()->SetAmbientLight(
        {1.0f, 1.0f, 1.0f, 0.6f});
#pragma endregion
}
} // namespace ExampleUi
