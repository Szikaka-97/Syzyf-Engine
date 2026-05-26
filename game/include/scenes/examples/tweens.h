#pragma once

#include "EasingFunctions.h"
#include "GameObject.h"
#include <Camera.h>
#include <ColorGrading.h>
#include <Fxaa.h>
#include <Light.h>
#include <LightSystem.h>
#include <Scene.h>
#include <Shader.h>
#include <Tonemapper.h>
#include <TweenSystem.h>
#include <fog/FogVolume.h>
#include <glm/trigonometric.hpp>
#include <physics/System.h>
#include <text/Font.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <ui/objects/custom/UiCircularBar.h>
#include <ui/systems/UiSystem.h>

namespace ExampleTweens {

class TweenedObject : public GameObject {
  private:
    TweenHandle tween;

  public:
    void OnEnable() {
        this->GlobalTransform().Scale() = glm::vec3(1.0f);

        auto* tweenSystem = this->GetScene()->GetComponent<TweenSystem>();

        TweenConfig config;
        config.initialValue = this->GlobalTransform().Scale().Value().x;
        config.targetValue = 2.0f;
        config.duration = 3.0f;
        config.easingFunction = Easing::outBounce;

        this->tween = std::move(
            tweenSystem->CreateTween(config).Bind([this](float newValue) {
                this->GlobalTransform().Scale() = glm::vec3(newValue);
                this->GlobalTransform().Rotation() =
                    glm::quat(glm::angleAxis(glm::radians(newValue * 180.0f),
                                             glm::vec3(0.0f, 0.0f, 1.0f)));
            }));
    }
};

class LoopingTweenedObject : public GameObject {
  private:
    TweenHandle tween;
    bool shrinking = false;

  public:
    void OnEnable() {
        auto* tweenSystem = this->GetScene()->GetComponent<TweenSystem>();

        TweenConfig config;
        config.initialValue = this->GlobalTransform().Scale().Value().x;
        config.targetValue = this->shrinking ? 2.0f : 1.0f;
        config.duration = 1.5f;
        config.easingFunction = Easing::inOutSine;

        this->tween = std::move(tweenSystem->CreateTween(config)
                                    .Bind([this](float newValue) {
                                        this->GlobalTransform().Scale() =
                                            glm::vec3(newValue);
                                    })
                                    .OnComplete([this, tweenSystem]() {
                                        this->shrinking = !this->shrinking;
                                        this->OnEnable();
                                    }));
    }
};

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<TweenSystem>();

#pragma region Ui
    // UiNode
    SceneNode* uiNode = mainScene.CreateNode("Ui Node");
    uiNode->AddObject<TweenedObject>();

    uiNode->AddObject<UiLayout>(glm::uvec2(200, 200), glm::uvec2(0, -150), 0,
                                AnchorPoint::BottomCenter);
    auto* uiVisual = uiNode->AddObject<UiVisual>(
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        mainScene.Resources()->Get<Texture2D>(
            "./res/textures/1147437805040054272.png",
            Texture2D::ColorTextureRGBA));
    uiVisual->colorHovered = {1.0f, 0.0f, 0.0f, 1.0f};
    uiVisual->colorClicked = {0.0f, 1.0f, 0.0f, 1.0f};
    uiNode->AddObject<UiInteractable>();

    // UiText
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

    SceneNode* uiTextRootNode = mainScene.CreateNode("UiTextRoot");
    uiTextRootNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    uiTextRootNode->AddObject<UiLayout>(glm::ivec2(140.0f, 100.0f),
                                        glm::ivec2(20.0f, 100.0f), 0,
                                        AnchorPoint::TopCenter);

    SceneNode* uiTextNode = mainScene.CreateNode(uiTextRootNode, "UiText");
    uiTextNode->AddObject<UiLayout>(glm::ivec2(100.0f), glm::ivec2(2.0f, -5.0f),
                                    1);
    auto* uiText =
        uiTextNode->AddObject<UiText>("Pooga\nSchnoz", openSansRegularFont);
    uiText->fontSize = 40.0f;
    uiText->color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
    uiTextRootNode->AddObject<LoopingTweenedObject>();

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
} // namespace ExampleTweens
