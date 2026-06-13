#pragma once

#include "Application.h"
#include "Bloom.h"
#include "Camera.h"
#include "ColorGrading.h"
#include "Fxaa.h"
#include "GameObject.h"
#include "GltfScene.h"
#include "JfaOutline.h"
#include "Light.h"
#include "LoadingScene.h"
#include "MaskEffects.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Tonemapper.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/systems/UiLayoutSystem.h"
#include "ui/systems/UiSystem.h"
#include "ui/widgets/UiOptionsMenu.h"

#include <SDL3/SDL_events.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <algorithm>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <physics/Body.h>
#include <physics/Helpers.h>
#include <physics/System.h>
#include <utility>
#include <vector>

namespace MainMenu {

inline SceneNode* FindNodeRecursive(SceneNode* node, const std::string& targetName) {
    if (node == nullptr) {
        return nullptr;
    }

    if (node->GetName() == targetName) {
        return node;
    }

    for (SceneNode* child : node->GetChildren()) {
        SceneNode* found = FindNodeRecursive(child, targetName);
        if (found != nullptr) {
            return found;
        }
    }

    return nullptr;
}

inline SceneNode* FindCameraNodeWithCameraObject(SceneNode* rootNode) {
    if (rootNode == nullptr) {
        return nullptr;
    }

    if (rootNode->GetObject<Camera>()) {
        return rootNode;
    }

    for (SceneNode* child : rootNode->GetChildren()) {
        if (SceneNode* foundNode = FindCameraNodeWithCameraObject(child)) {
            return foundNode;
        }
    }

    return nullptr;
}

inline bool IsNodeInsideSubtree(SceneNode* node, SceneNode* subtreeRoot) {
    if (node == nullptr || subtreeRoot == nullptr) {
        return false;
    }

    return node == subtreeRoot || node->IsChildOf(subtreeRoot);
}

inline bool IsMenuPointLightNodeName(const std::string& name) {
    return
        name == "Point" ||
        name.rfind("Point.", 0) == 0;
}

inline int GetPointLightIndex(const std::string& name) {
    try {
        if (name == "Point") {
            return 0;
        }

        const std::string pointPrefix = "Point.";

        if (name.rfind(pointPrefix, 0) == 0) {
            return std::stoi(name.substr(pointPrefix.size()));
        }
    }
    catch (...) {
    }

    return 999999;
}

inline void CollectPointLightsRecursive(SceneNode* node, std::vector<SceneNode*>& pointLights) {
    if (node == nullptr) {
        return;
    }

    if (IsMenuPointLightNodeName(node->GetName())) {
        pointLights.push_back(node);
    }

    for (SceneNode* child : node->GetChildren()) {
        CollectPointLightsRecursive(child, pointLights);
    }
}

inline glm::quat LookAtRotation(
    const glm::vec3& cameraPosition,
    const glm::vec3& targetPosition
) {
    glm::vec3 direction = targetPosition - cameraPosition;

    if (glm::length(direction) < 0.0001f) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    glm::vec3 forward = glm::normalize(direction);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (glm::abs(glm::dot(forward, up)) > 0.98f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 view = glm::lookAt(
        cameraPosition,
        targetPosition,
        up
    );

    return glm::quat_cast(glm::inverse(view)) *
        glm::quat(glm::radians(glm::vec3(0.0f, 180.0f, 0.0f)));
}

inline bool ContainsSceneNode(const std::vector<SceneNode*>& nodes, SceneNode* searchedNode) {
    for (SceneNode* node : nodes) {
        if (node == searchedNode) {
            return true;
        }
    }

    return false;
}

inline void CollectMenuLightNodesRecursive(SceneNode* node, std::vector<SceneNode*>& lightNodes) {
    if (node == nullptr) {
        return;
    }

    Light* existingLight = nullptr;

    if (node->TryGetObject<Light>(existingLight) ||
        IsMenuPointLightNodeName(node->GetName())) {
        if (!ContainsSceneNode(lightNodes, node)) {
            lightNodes.push_back(node);
        }
        }

    for (SceneNode* child : node->GetChildren()) {
        CollectMenuLightNodesRecursive(child, lightNodes);
    }
}

class MainMenuLights : public GameObject {
  private:
    std::vector<Light*> lights;

  public:
    void Awake() {
        std::vector<SceneNode*> lightNodes;

        CollectMenuLightNodesRecursive(GetNode(), lightNodes);

        std::sort(lightNodes.begin(), lightNodes.end(), [](SceneNode* a, SceneNode* b) {
            return GetPointLightIndex(a->GetName()) < GetPointLightIndex(b->GetName());
        });

        for (SceneNode* lightNode : lightNodes) {
            Light* light = nullptr;

            if (!lightNode->TryGetObject<Light>(light)) {
                light = lightNode->AddObject<Light>(
                    Light::PointLight(
                        glm::vec3(1.0f, 0.72f, 0.38f),
                        14.0f,
                        5.0f,
                        0.045f,
                        0.012f
                    )
                );
            }

            if (light->GetType() == Light::LightType::Directional) {
                light->Set(Light::DirectionalLight(
                    glm::vec3(1.0f, 0.82f, 0.55f),
                    0.7f
                ));
            } else {
                light->Set(Light::PointLight(
                    glm::vec3(1.0f, 0.72f, 0.38f),
                    14.0f,
                    5.0f,
                    0.045f,
                    0.012f
                ));
            }

            light->SetShadowCasting(false);

            lights.push_back(light);

            spdlog::info(
                "MainMenu: enabled light {}",
                lightNode->GetName()
            );
        }

        SceneNode* fillLightNode =
            GetScene()->CreateNode(GetNode(), "Main Menu Warm Fill Directional");

        fillLightNode->LocalTransform().Rotation() =
            glm::quat(glm::radians(glm::vec3(-45.0f, -25.0f, 0.0f)));

        Light* fillLight = fillLightNode->AddObject<Light>(
            Light::DirectionalLight(
                glm::vec3(1.0f, 0.78f, 0.48f),
                1.2f
            )
        );

        fillLight->SetShadowCasting(false);
        lights.push_back(fillLight);

        spdlog::info(
            "MainMenu: enabled {} lights from main_Menu.glb plus warm fill light.",
            lights.size()
        );
    }
};

enum class MenuBottleAction {
    None,
    Start,
    Settings,
    Quit
};

class MainMenuController : public GameObject {
  public:
    UiLayout* playTextLayout = nullptr;
    UiLayout* optionsTextLayout = nullptr;
    UiLayout* quitTextLayout = nullptr;

    SceneNode* playBottleNode = nullptr;
    SceneNode* optionsBottleNode = nullptr;
    SceneNode* quitBottleNode = nullptr;

    SceneNode* runtimeCameraNode = nullptr;
    SceneNode* mainMenuCameraNode = nullptr;
    SceneNode* settingsCameraNode = nullptr;

    Camera* camera = nullptr;
    UiOptionsMenu* optionsController = nullptr;

    Scene* loadingScene = nullptr;

    float interactionDistance = 100.0f;

    MainMenuController() {
        this->loadingScene = Scene::CreateStandaloneScene();
        LoadingScene::InitScene(*loadingScene);
    }

    ~MainMenuController() {
        ClearHoverHighlight();

        if (this->loadingScene != nullptr) {
            delete this->loadingScene;
        }
    }

    bool FocusCameraOnBlenderCamera(SceneNode* blenderCameraRootNode) {
        if (runtimeCameraNode == nullptr) {
            spdlog::warn("MainMenu: runtime camera node not found.");
            return false;
        }

        if (blenderCameraRootNode == nullptr) {
            return false;
        }

        SceneNode* blenderCameraDataNode =
            FindCameraNodeWithCameraObject(blenderCameraRootNode);

        Camera* runtimeCamera =
            runtimeCameraNode->GetObject<Camera>();

        Camera* blenderCamera = nullptr;

        if (blenderCameraDataNode != nullptr) {
            blenderCamera = blenderCameraDataNode->GetObject<Camera>();
        }

        glm::vec3 cameraPosition =
            blenderCameraRootNode->GlobalTransform().Position().Value();

        glm::quat cameraRotation =
            blenderCameraRootNode->GlobalTransform().Rotation().Value() *
            glm::quat(glm::radians(glm::vec3(180.0f, 0.0f, 0.0f)));

        runtimeCameraNode->GlobalTransform().Position() =
            cameraPosition;

        runtimeCameraNode->GlobalTransform().Rotation() =
            cameraRotation;

        if (runtimeCamera != nullptr && blenderCamera != nullptr) {
            runtimeCamera->MakePerspective(
                blenderCamera->GetFov(),
                blenderCamera->GetAspectRatio(),
                blenderCamera->GetNearPlane(),
                blenderCamera->GetFarPlane()
            );
        } else if (runtimeCamera != nullptr) {
            runtimeCamera->MakePerspective(
                45.0f,
                16.0f / 9.0f,
                0.1f,
                300.0f
            );
        }

        if (runtimeCamera != nullptr) {
            runtimeCamera->SetAsMainCamera();
            camera = runtimeCamera;
        }

        return true;
    }

    void FocusMainMenuCamera() {
        if (!FocusCameraOnBlenderCamera(mainMenuCameraNode)) {
            spdlog::warn("MainMenu: MainMenuCamera not found, using fallback camera transform.");
        }
    }

    void FocusSettingsCamera() {
        if (!FocusCameraOnBlenderCamera(settingsCameraNode)) {
            spdlog::warn("MainMenu: SettingsCamera not found, using current camera transform.");
        }
    }

    void OpenOptions() {
        if (optionsController != nullptr) {
            optionsController->SetVisible(true);
        }

        ClearHoverHighlight();
        HideBottleTexts();
        FocusSettingsCamera();
    }

    void CloseOptions() {
        if (optionsController != nullptr) {
            optionsController->SetVisible(false);
        }

        FocusMainMenuCamera();
    }

    void Update() {
        bool optionsVisible =
            optionsController != nullptr &&
            optionsController->IsVisible();

        if (!optionsVisible) {
            UpdateProjectedText(playTextLayout, playBottleNode, glm::ivec2(0, 0));
            UpdateProjectedText(optionsTextLayout, optionsBottleNode, glm::ivec2(0, 0));
            UpdateProjectedText(quitTextLayout, quitBottleNode, glm::ivec2(0, 0));

            MenuBottleAction hoveredAction = GetBottleActionUnderMouse();
            UpdateHoverHighlight(hoveredAction);

            if (GetScene() != nullptr &&
                GetScene()->Input() != nullptr &&
                GetScene()->Input()->ButtonDown(MouseButton::Left)) {
                ActivateBottleAction(hoveredAction);
            }
        } else {
            ClearHoverHighlight();
            HideBottleTexts();

            if (GetScene()->Input()->KeyDown(Key::Escape)) {
                CloseOptions();
            }
        }
    }

  private:
    SceneNode* highlightedNode = nullptr;
    std::vector<std::pair<MeshRenderer*, uint8_t>> highlightedRenderers;

    void HideBottleTexts() {
        HideProjectedLayout(playTextLayout);
        HideProjectedLayout(optionsTextLayout);
        HideProjectedLayout(quitTextLayout);
    }

    void HideProjectedLayout(UiLayout* layout) {
        if (layout == nullptr) {
            return;
        }

        layout->offset = glm::ivec2(9999, 9999);
    }

    void ActivateBottleAction(MenuBottleAction action) {
        switch (action) {
            case MenuBottleAction::Start:
                if (this->loadingScene != nullptr) {
                    Application::Get()->RequestSceneChange(this->loadingScene);
                    this->loadingScene = nullptr;
                }
                break;

            case MenuBottleAction::Settings:
                OpenOptions();
                break;

            case MenuBottleAction::Quit: {
                SDL_Event quitEvent{};
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
                break;
            }

            case MenuBottleAction::None:
                break;
        }
    }

    MenuBottleAction GetBottleActionUnderMouse() {
        SceneNode* hitNode = RaycastBottleUnderMouse();

        if (hitNode == nullptr) {
            return MenuBottleAction::None;
        }

        if (IsNodeInsideSubtree(hitNode, playBottleNode)) {
            return MenuBottleAction::Start;
        }

        if (IsNodeInsideSubtree(hitNode, optionsBottleNode)) {
            return MenuBottleAction::Settings;
        }

        if (IsNodeInsideSubtree(hitNode, quitBottleNode)) {
            return MenuBottleAction::Quit;
        }

        return MenuBottleAction::None;
    }

    SceneNode* GetBottleNodeForAction(MenuBottleAction action) {
        switch (action) {
            case MenuBottleAction::Start:
                return playBottleNode;

            case MenuBottleAction::Settings:
                return optionsBottleNode;

            case MenuBottleAction::Quit:
                return quitBottleNode;

            case MenuBottleAction::None:
                return nullptr;
        }

        return nullptr;
    }

    SceneNode* RaycastBottleUnderMouse() {
        if (GetScene() == nullptr || camera == nullptr) {
            return nullptr;
        }

        auto* physics = GetScene()->GetComponent<Physics::System>();

        if (physics == nullptr) {
            return nullptr;
        }

        glm::vec3 rayOrigin;
        glm::vec3 rayDirection;

        if (!BuildMouseRay(rayOrigin, rayDirection)) {
            return nullptr;
        }

        Physics::RayCastPayload hit =
            physics->CastRay(
                rayOrigin,
                rayDirection * interactionDistance
            );

        if (!hit.hasHit || hit.node == nullptr) {
            return nullptr;
        }

        if (IsNodeInsideSubtree(hit.node, playBottleNode) ||
            IsNodeInsideSubtree(hit.node, optionsBottleNode) ||
            IsNodeInsideSubtree(hit.node, quitBottleNode)) {
            return hit.node;
        }

        return nullptr;
    }

    bool BuildMouseRay(glm::vec3& outOrigin, glm::vec3& outDirection) {
        if (camera == nullptr ||
            GetScene() == nullptr ||
            GetScene()->Input() == nullptr ||
            GetScene()->GetGraphics() == nullptr) {
            return false;
        }

        glm::vec2 mousePosition = GetScene()->Input()->GetMousePosition();
        glm::vec2 screenSize = GetScene()->GetGraphics()->GetScreenResolution();

        float ndcX = (2.0f * mousePosition.x) / screenSize.x - 1.0f;
        float ndcY = 1.0f - (2.0f * mousePosition.y) / screenSize.y;

        glm::vec4 clipSpacePosition = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

        glm::vec4 viewSpacePosition =
            glm::inverse(camera->ProjectionMatrix()) * clipSpacePosition;

        viewSpacePosition.z = -1.0f;
        viewSpacePosition.w = 0.0f;

        glm::vec4 worldDirection =
            glm::inverse(camera->ViewMatrix()) * viewSpacePosition;

        outOrigin = camera->GlobalTransform().Position().Value();
        outDirection = glm::normalize(glm::vec3(worldDirection));

        return true;
    }

    void ClearHoverHighlight() {
        for (auto& highlightedRenderer : highlightedRenderers) {
            if (highlightedRenderer.first) {
                highlightedRenderer.first->maskFlags = highlightedRenderer.second;
            }
        }

        highlightedRenderers.clear();
        highlightedNode = nullptr;
    }

    void CollectMeshRenderersRecursive(SceneNode* node, std::vector<MeshRenderer*>& renderers) {
        if (node == nullptr) {
            return;
        }

        if (auto* renderer = node->GetObject<MeshRenderer>()) {
            renderers.push_back(renderer);
        }

        for (SceneNode* child : node->GetChildren()) {
            CollectMeshRenderersRecursive(child, renderers);
        }
    }

    void SetHoverHighlight(SceneNode* node) {
        if (node == highlightedNode) {
            return;
        }

        ClearHoverHighlight();

        if (node == nullptr) {
            return;
        }

        std::vector<MeshRenderer*> renderers;
        CollectMeshRenderersRecursive(node, renderers);

        for (auto* renderer : renderers) {
            if (renderer == nullptr) {
                continue;
            }

            highlightedRenderers.push_back({renderer, renderer->maskFlags});
            renderer->maskFlags = renderer->maskFlags | MaskEffectBits::Jfa;
        }

        highlightedNode = node;
    }

    void UpdateHoverHighlight(MenuBottleAction hoveredAction) {
        SetHoverHighlight(GetBottleNodeForAction(hoveredAction));
    }

    void UpdateProjectedText(
        UiLayout* layout,
        SceneNode* targetNode,
        const glm::ivec2& extraOffset
    ) {
        if (layout == nullptr || targetNode == nullptr || camera == nullptr) {
            return;
        }

        if (GetScene() == nullptr || GetScene()->GetGraphics() == nullptr) {
            return;
        }

        glm::vec3 worldPosition =
            targetNode->LocalTransform().Position().Value();

        glm::vec4 clipPosition =
            camera->ViewProjectionMatrix() *
            glm::vec4(worldPosition, 1.0f);

        if (clipPosition.w <= 0.001f) {
            layout->offset = glm::ivec2(9999, 9999);
            return;
        }

        glm::vec3 ndcPosition =
            glm::vec3(clipPosition) / clipPosition.w;

        if (ndcPosition.x < -1.2f || ndcPosition.x > 1.2f ||
            ndcPosition.y < -1.2f || ndcPosition.y > 1.2f) {
            layout->offset = glm::ivec2(9999, 9999);
            return;
        }

        glm::vec2 resolution =
            GetScene()->GetGraphics()->GetScreenResolution();

        float scaleFactor =
            resolution.y /
            static_cast<float>(UiLayoutSystem::VIRTUAL_RESOLUTION.y);

        glm::vec2 screenPosition = {
            (ndcPosition.x * 0.5f + 0.5f) * resolution.x,
            (1.0f - (ndcPosition.y * 0.5f + 0.5f)) * resolution.y
        };

        glm::vec2 virtualOffset =
            (screenPosition - resolution * 0.5f) / scaleFactor;

        layout->offset = glm::ivec2(
            static_cast<int>(virtualOffset.x),
            static_cast<int>(virtualOffset.y)
        ) + extraOffset;
    }
};

inline UiText* CreateBottleText(
    Scene& mainScene,
    SceneNode* parent,
    const std::string& name,
    const std::string& text,
    Font* font,
    UiLayout** outLayout
) {
    SceneNode* textNode = mainScene.CreateNode(parent, name);

    UiLayout* layout = textNode->AddObject<UiLayout>(
        glm::uvec2(420, 130),
        glm::ivec2(9999, 9999),
        30,
        AnchorPoint::Center
    );

    UiText* uiText = textNode->AddObject<UiText>(text, font);
    uiText->fontSize = 46.0f;
    uiText->alignment = TextAlignment::Middle;
    uiText->maxWidth = 400.0f;
    uiText->color = glm::vec4(1.2f, 0.3f, 0.0f, 1.0f);

    if (outLayout != nullptr) {
        *outLayout = layout;
    }

    return uiText;
}

inline void AddBottleMeshPhysicsRecursive(SceneNode* node) {
    if (node == nullptr) {
        return;
    }

    if (MeshRenderer* renderer = node->GetObject<MeshRenderer>()) {
        if (renderer->GetMesh() != nullptr && node->GetObject<Physics::Body>() == nullptr) {
            auto* body = node->AddObject<Physics::Body>(
                JPH::BodyCreationSettings{
                    Physics::MeshShape(renderer->GetMesh()),
                    JPH::RVec3::sZero(),
                    JPH::Quat::sIdentity(),
                    JPH::EMotionType::Static,
                    Physics::Layers::NON_MOVING
                }
            );

            body->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
        }
    }

    for (SceneNode* child : node->GetChildren()) {
        AddBottleMeshPhysicsRecursive(child);
    }
}

inline void AddBottleMeshPhysics(SceneNode* bottleNode) {
    if (bottleNode == nullptr) {
        return;
    }

    AddBottleMeshPhysicsRecursive(bottleNode);
}

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<Physics::System>();

    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->LocalTransform().Position() =
        glm::vec3(-4.9f, 3.2f, 10.8f);
    cameraNode->LocalTransform().Rotation() =
        LookAtRotation(
            cameraNode->LocalTransform().Position().Value(),
            glm::vec3(-4.85f, 1.9f, 5.2f)
        );

    auto* camera = cameraNode->AddObject<Camera>(
        Camera::Perspective(45.0f, 16.0f / 9.0f, 0.1f, 300.0f));
    camera->SetAsMainCamera();

    cameraNode->AddObject<MaskEffects>();
    auto* jfa = cameraNode->AddObject<JfaOutline>();
    jfa->outlineThickness = 4.0f;
    jfa->outlineColor = {1.0f, 29.0f / 255.0f, 29.0f / 255.0f};

    cameraNode->AddObject<Bloom>();
    cameraNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    cameraNode->AddObject<ColorGrading>();
    cameraNode->AddObject<Fxaa>();

    GltfScene* menuScene =
        mainScene.Resources()->Get<GltfScene>("./res/models/rooms/main_Menu.glb");

    SceneNode* menuModelNode = nullptr;

    if (menuScene != nullptr) {
        menuModelNode =
            menuScene->Instantiate(&mainScene, mainScene.root, "Main Menu Model");
    } else {
        spdlog::error("MainMenu: missing model ./res/models/rooms/main_Menu.glb");
    }

    if (menuModelNode != nullptr) {
        menuModelNode->AddObject<MainMenuLights>();
    }

    TextureParams fontParams = {.channels = TextureChannels::RGB,
                                .colorSpace = TextureColor::Linear,
                                .format = TextureFormat::Ubyte,
                                .wrapU = TextureWrap::Clamp,
                                .wrapV = TextureWrap::Clamp,
                                .minFilter = TextureFilter::Linear,
                                .magFilter = TextureFilter::Linear};
    Texture2D* fontAtlas = mainScene.Resources()->Get<Texture2D>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.png", fontParams);
    Font* font = mainScene.Resources()->Get<Font>(
        "./res/fonts/OpenSans-Regular/OpenSans-Regular.json", fontAtlas, true);

    SceneNode* logicNode = mainScene.CreateNode("MainMenu Logic");
    auto* controller = logicNode->AddObject<MainMenuController>();
    controller->camera = camera;
    controller->runtimeCameraNode = cameraNode;

    if (menuModelNode != nullptr) {
        controller->mainMenuCameraNode =
            FindNodeRecursive(menuModelNode, "MainMenuCamera");
        controller->settingsCameraNode =
            FindNodeRecursive(menuModelNode, "SettingsCamera");

        if (controller->mainMenuCameraNode == nullptr) {
            spdlog::warn("MainMenu: node MainMenuCamera not found in model");
        }

        if (controller->settingsCameraNode == nullptr) {
            spdlog::warn("MainMenu: node SettingsCamera not found in model");
        }

        controller->playBottleNode =
            FindNodeRecursive(menuModelNode, "start_bottle");
        controller->optionsBottleNode =
            FindNodeRecursive(menuModelNode, "settings_bottle");
        controller->quitBottleNode =
            FindNodeRecursive(menuModelNode, "quit_bottle");

        if (controller->playBottleNode == nullptr) {
            spdlog::warn("MainMenu: node start_bottle not found in model");
        }

        if (controller->optionsBottleNode == nullptr) {
            spdlog::warn("MainMenu: node settings_bottle not found in model");
        }

        if (controller->quitBottleNode == nullptr) {
            spdlog::warn("MainMenu: node quit_bottle not found in model");
        }

        AddBottleMeshPhysics(controller->playBottleNode);
        AddBottleMeshPhysics(controller->optionsBottleNode);
        AddBottleMeshPhysics(controller->quitBottleNode);

        controller->FocusMainMenuCamera();
    }

    SceneNode* bottleTextsGroup =
        mainScene.CreateNode("Main Menu Bottle Texts");

    UiOptionsMenu* optionsUi = OptionsMenu::Build(mainScene, font);
    controller->optionsController = optionsUi;

    optionsUi->onBackClicked = [controller]() {
        controller->CloseOptions();
    };
}
} // namespace MainMenu