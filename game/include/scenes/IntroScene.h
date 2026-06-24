#pragma once

#include "Application.h"
#include "Camera.h"
#include "GameObject.h"
#include "Scene.h"
#include "TimeSystem.h"
#include "scenes/MainMenuScene.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/objects/UiVisual.h"
#include "ui/systems/UiSystem.h"

namespace IntroScene {

class IntroController : public GameObject {
private:
    float m_Elapsed = 0.0f;

public:
    IntroController() = default;

    void Update() {
        m_Elapsed += Time::Delta();

        bool skip = m_Elapsed > 2.0f &&
                    (GetScene()->Input()->KeyDown(Key::Space) ||
                     GetScene()->Input()->ButtonDown(MouseButton::Left));

        if (skip || m_Elapsed > 22.0f) {
            Application::Get()->RequestSceneBuild(
                [](Scene* s) { MainMenu::InitScene(*s); });
        }
    }
};

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<UiSystem>();

    SceneNode* cameraNode = mainScene.CreateNode("Camera");
    auto* camera = cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    camera->SetAsMainCamera();

    SceneNode* bgNode = mainScene.CreateNode("Background");
    bgNode->AddObject<UiLayout>(glm::uvec2(1920, 1080), glm::ivec2(0, 0), 0,
                                AnchorPoint::Center);
    bgNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    TextureParams fontParams = {.channels    = TextureChannels::RGB,
                                .colorSpace  = TextureColor::Linear,
                                .format      = TextureFormat::Ubyte,
                                .wrapU       = TextureWrap::Clamp,
                                .wrapV       = TextureWrap::Clamp,
                                .minFilter   = TextureFilter::Linear,
                                .magFilter   = TextureFilter::Linear};

    Texture2D* fontAtlas = mainScene.Resources()->Get<Texture2D>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.png", fontParams);
    Font* font = mainScene.Resources()->Get<Font>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.json", fontAtlas);

    SceneNode* textNode = mainScene.CreateNode("Intro Text");
    textNode->AddObject<UiLayout>(glm::uvec2(700, 300), glm::ivec2(0, -60), 1,
                                  AnchorPoint::Center);
    auto* text = textNode->AddObject<UiText>(
        "A good day to you, Sir! What a memorable day has come upon us!\n"
        "Venture deep underground and defeat all your foes using your own\n"
        "crafted home brews to save your ancestors' moonshining legacy\n"
        "and discover the legendary recipe for the moonshine of immortality!",
        font);
    text->fontSize   = 22.0f;
    text->alignment  = TextAlignment::Middle;
    text->color      = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    text->maxWidth   = 700.0f;

    SceneNode* hintNode = mainScene.CreateNode("Hint Text");
    hintNode->AddObject<UiLayout>(glm::uvec2(550, 40), glm::ivec2(0, 200), 1,
                                  AnchorPoint::Center);
    auto* hint = hintNode->AddObject<UiText>("[ Space / Click — continue ]", font);
    hint->fontSize  = 16.0f;
    hint->alignment = TextAlignment::Middle;
    hint->color     = glm::vec4(0.55f, 0.55f, 0.55f, 1.0f);
    hint->maxWidth  = 1000;

    SceneNode* logicNode = mainScene.CreateNode("Intro Logic");
    logicNode->AddObject<IntroController>();
}

}