#pragma once

#include "Application.h"
#include "Camera.h"
#include "GameObject.h"
#include "LoadingScene.h"
#include "Scene.h"
#include "Texture.h"
#include "TimeSystem.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiVisual.h"
#include "ui/systems/UiSystem.h"

#include <array>
#include <cstdio>
#include <string>

namespace IntroScene {

class IntroFramePlayer : public GameObject {
  public:
    UiVisual* frameVisual = nullptr;
    Scene* nextScene = nullptr;

    static constexpr int FrameCount = 53;
    static constexpr float FramesPerSecond = 3.0f;

    IntroFramePlayer(UiVisual* frameVisual, Scene* nextScene)
        : frameVisual(frameVisual), nextScene(nextScene) {}

    void Awake() {
        LoadFrames();
        ShowFrame(0);
    }

    void Update() {
        if (frameVisual == nullptr || frames.empty()) {
            GoToNextScene();
            return;
        }

        elapsed += Time::UnscaledDelta();

        int frameIndex = static_cast<int>(elapsed * FramesPerSecond);

        if (frameIndex >= static_cast<int>(frames.size())) {
            GoToNextScene();
            return;
        }

        ShowFrame(frameIndex);
    }

    ~IntroFramePlayer() {
        if (nextScene != nullptr) {
            delete nextScene;
            nextScene = nullptr;
        }
    }

  private:
    std::array<Texture2D*, FrameCount> frames{};
    int currentFrameIndex = -1;
    float elapsed = 0.0f;

    void LoadFrames() {
        TextureParams frameParams = Texture2D::ColorTextureRGBA;

        for (int i = 0; i < FrameCount; ++i) {
            char path[128];
            std::snprintf(
                path,
                sizeof(path),
                "./res/textures/intro/intro_%04d.png",
                i + 1
            );

            frames[i] = GetScene()->Resources()->Get<Texture2D>(
                std::string(path),
                frameParams
            );
        }
    }

    void ShowFrame(int frameIndex) {
        if (frameIndex == currentFrameIndex) {
            return;
        }

        if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) {
            return;
        }

        if (frames[frameIndex] == nullptr) {
            GoToNextScene();
            return;
        }

        frameVisual->texture = frames[frameIndex];
        currentFrameIndex = frameIndex;
    }

    void GoToNextScene() {
        if (nextScene == nullptr) {
            nextScene = Scene::CreateStandaloneScene();
            LoadingScene::InitScene(*nextScene);
        }

        Scene* sceneToOpen = nextScene;
        nextScene = nullptr;
        Application::Get()->RequestSceneChange(sceneToOpen);
    }
};

inline void InitScene(Scene& mainScene, Scene* nextScene) {
    mainScene.AddComponent<UiSystem>();

    SceneNode* cameraNode = mainScene.CreateNode("Intro Camera");
    Camera* camera = cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f)
    );
    camera->SetAsMainCamera();

    SceneNode* backgroundNode = mainScene.CreateNode("Intro Black Background");
    backgroundNode->AddObject<UiLayout>(
        glm::uvec2(1920, 1080),
        glm::ivec2(0, 0),
        0,
        AnchorPoint::Center
    );
    backgroundNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    SceneNode* frameNode = mainScene.CreateNode("Intro Frame");
    frameNode->AddObject<UiLayout>(
        glm::uvec2(1920, 1080),
        glm::ivec2(0, 0),
        1,
        AnchorPoint::Center
    );
    UiVisual* frameVisual = frameNode->AddObject<UiVisual>(glm::vec4(1.0f));

    SceneNode* logicNode = mainScene.CreateNode("Intro Logic");
    logicNode->AddObject<IntroFramePlayer>(frameVisual, nextScene);
}

} // namespace IntroScene
