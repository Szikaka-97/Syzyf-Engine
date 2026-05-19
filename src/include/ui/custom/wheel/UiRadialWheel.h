#pragma once

#include "Camera.h"
#include "Debug.h"
#include "Framebuffer.h"
#include "GameObject.h"
#include "GltfImporter.h"
#include "InputSystem.h"
#include "Material.h"
#include "MeshRenderer.h"
#include "TweenSystem.h"
#include "Viewport.h"
#include "ui/custom/wheel/UiWheel.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/objects/UiVisual.h"
#include "Graphics.h"
#include "ui/systems/UiLayoutSystem.h"

#include <cmath>
#include <imgui.h>
#include <filesystem>

namespace fs = std::filesystem;

class UiRadialWheel : public GameObject, public ImGuiDrawable {
public:
    int numberOfSlices = 5;

    float innerRadius = 25.0f;
    float outerRadius = 300.0f;
    float gapWidth = 0.02f;
    
    float alphaMultiplier = 0.3f;
    glm::vec3 hoverColorAdd = glm::vec3(0.2f);

    std::unique_ptr<Material> material;

    // Tooltip
    std::vector<std::string> tooltipDescriptions;

private:
    struct ItemSlot {
        std::unique_ptr<Viewport> viewport;
        SceneNode* cameraNode = nullptr;
        SceneNode* itemNode = nullptr;
        SceneNode* uiNode = nullptr;
    };

    std::vector<ItemSlot> itemSlots;

    int hoveredSlice = -1;

    // Tooltip
    SceneNode* tooltipNode = nullptr;
    float hoverTimer = 0.0f;
    float hoverDelay = 0.05f;
    int lastHoveredSlice = -1;

    TweenSystem* tweenSystem = nullptr;
    TweenHandle tooltipTween;

    bool isTooltipVisible = false;
    float currentTooltipAlpha = 0.0f;

public:
    UiRadialWheel() {
        // Tooltip setup
        this->tooltipNode = this->GetScene()->CreateNode("Tooltip Node");
        auto* visual = this->tooltipNode->AddObject<UiVisual>();
        auto* layout = this->tooltipNode->AddObject<UiLayout>();
        layout->size = {400, 400};
        layout->zIndex = 2;
        auto* text = this->tooltipNode->AddObject<UiText>();
        text->maxWidth = layout->size.x;

        visual->color = {0.0, 0.0, 0.0, 0.2};
        
        TextureParams fontTextureParams = {
            .channels = TextureChannels::RGB,
            .colorSpace = TextureColor::Linear,

            .format = TextureFormat::Ubyte,
            .wrapU = TextureWrap::Clamp,
            .wrapV = TextureWrap::Clamp,
            .minFilter = TextureFilter::Linear,
            .magFilter = TextureFilter::Linear,
        };

        Texture2D* openSansFontAtlasTexture = GetScene()->Resources()->Get<Texture2D>("./res/fonts/OpenSans-Regular/OpenSans-Regular.png", fontTextureParams);
        Font* openSansRegularFont = GetScene()->Resources()->Get<Font>(
            "./res/fonts/OpenSans-Regular/OpenSans-Regular.json",
            openSansFontAtlasTexture
        );
        text->font = openSansRegularFont;
        text->text = "Pooga Pooga Pogaaadskadskdsakjdjsa Poooga pooooooga poooga poooga poasdopsada. POOOOOOga podaooga. Poga pooooga";
    } 

    void SetItemModels(const std::vector<fs::path>& gltfPaths) {
        uint32_t ui3DLayer = 2;
        uint32_t mask = (1 << ui3DLayer);

        numberOfSlices = gltfPaths.size();
        itemSlots.clear();

        GltfImporter importer;

        for (int i = 0; i < numberOfSlices; i++) {
            ItemSlot slot;

            slot.viewport = std::make_unique<Viewport>();
            slot.viewport->GetFramebuffer()->CreateColorAttachment(true, false);
            slot.viewport->GetFramebuffer()->CreateDepthAttachment(true, false);
            slot.viewport->SetSize(glm::uvec2(256, 256));

            slot.cameraNode = GetScene()->CreateNode("ItemCamera_" + std::to_string(i));
            Camera* camera = slot.cameraNode->AddObject<Camera>(Camera::Perspective(60.0f, 1.0f, 0.01f, 50.0f));

            slot.cameraNode->GlobalTransform().Position() = glm::vec3(0.0f, -500.0f + (i * 10.0f), 0.0f);

            camera->SetRenderTarget(slot.viewport.get());
            camera->SetLayerMask(mask);

            slot.itemNode = GetScene()->CreateNode("ItemPivot_" + std::to_string(i));
            slot.itemNode->GlobalTransform().Position() = glm::vec3(0.0f, -500.0f + (i * 10.0f), 0.2f);

            importer.LoadScene(GetScene(), gltfPaths[i], "ItemModel_" + std::to_string(i), slot.itemNode);
            SetLayerRecursive(slot.itemNode, ui3DLayer);

            slot.uiNode = GetScene()->CreateNode(GetNode(), "ItemVisual_" + std::to_string(i));
            slot.uiNode->AddObject<WheelTag>();
            UiVisual* visual = slot.uiNode->AddObject<UiVisual>();
            visual->texture = (Texture2D*)slot.viewport->GetFramebuffer()->GetColorTexture();
            visual->color.a = 1.0f;
            visual->SetEnabled(false);

            UiLayout* layout = slot.uiNode->AddObject<UiLayout>();
            layout->size = glm::ivec2(150, 150);
            layout->anchorPoint = AnchorPoint::Center;
            layout->zIndex = 1;

            itemSlots.push_back(std::move(slot));
        }
    }

    void Update() {
        if (!tweenSystem) {
            tweenSystem = GetScene()->GetComponent<TweenSystem>();
        }

        InputSystem* input = GetScene()->Input();
        auto* layout = GetNode()->GetObject<UiLayout>();

        if (!material || !input || !layout) return;

        glm::vec3 globalPosition = GetNode()->GlobalTransform().Position();
        glm::vec2 wheelCenter = glm::vec2(globalPosition.x, globalPosition.y);

        glm::vec2 mousePosition = input->GetMousePosition();
        glm::vec2 relativeMouse = mousePosition - wheelCenter;
        float distance = glm::length(relativeMouse);

        glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
        float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

        float scaledInnerRadius = innerRadius * scaleFactor;
        float scaledOuterRadius = outerRadius * scaleFactor;

        bool isWheelVisible = false;
        if (auto* visual = GetNode()->GetObject<UiVisual>()) {
            isWheelVisible = visual->IsEnabled() && visual->color.a > 0.0f;
        }

        if (isWheelVisible && distance >= scaledInnerRadius && distance <= scaledOuterRadius) {
            float angle = std::atan2(-relativeMouse.y, relativeMouse.x);

            if (angle < 0.0f) {
                angle += glm::two_pi<float>();
            }

            float sliceAngle = glm::two_pi<float>() / numberOfSlices;
            hoveredSlice = static_cast<int>(angle / sliceAngle);

            if (input->ButtonDown(MouseButton::Left)) {
                spdlog::info("Clicked slice {}", hoveredSlice);
            }
        } else {
            hoveredSlice = -1;
        }

        if (hoveredSlice != lastHoveredSlice) {
            hoverTimer = 0.0f;
            lastHoveredSlice = hoveredSlice;
        }

        if (hoveredSlice >= 0) {
            hoverTimer += Time::Delta();
        }

        float maxRadius = layout->size.x / 2.0f;
        material->SetValue("innerRadiusNorm", innerRadius / maxRadius);
        material->SetValue("outerRadiusNorm", outerRadius / maxRadius);
        material->SetValue("gapWidth", gapWidth);
        material->SetValue("alphaMultiplier", alphaMultiplier);
        material->SetValue("hoverColorAdd", hoverColorAdd);

        material->SetValue("hoveredSlice", (unsigned int)hoveredSlice);
        material->SetValue("numberOfSlices", (unsigned int)numberOfSlices);

        this->RenderItems();

        this->HandleTooltip(mousePosition);
    }

    void DrawImGui() {
        ImGui::DragFloat("Inner Radius", &innerRadius, 1.0f, 0.0f, outerRadius);
        ImGui::DragFloat("Outer Radius", &outerRadius, 1.0f, innerRadius, 2000.0f);
        ImGui::DragFloat("Gap Width", &gapWidth, 0.001f, 0.0f, 0.5f);
        ImGui::DragFloat("Alpha Mult", &alphaMultiplier, 0.01f, 0.0f, 1.0f);
        ImGui::ColorEdit3("Hover Color Add", &hoverColorAdd[0]);
    }

private:
    void HandleTooltip(const glm::vec2& mousePosition) {
        bool shouldBeVisible = (this->hoveredSlice >= 0 && this->IsEnabled() && this->hoverTimer >= this->hoverDelay);

        if (shouldBeVisible != this->isTooltipVisible) {
            this->isTooltipVisible = shouldBeVisible;

            if (this->tweenSystem) {
                if (this->tweenSystem->IsValid(this->tooltipTween)) this->tooltipTween.SetPlaying(false);

                float targetAlpha = shouldBeVisible ? 1.0f : 0.0f;

                float diff = std::abs(targetAlpha - this->currentTooltipAlpha);
                float duration = 0.2f * diff;

                TweenConfig config = {
                    .initialValue = this->currentTooltipAlpha,
                    .targetValue = targetAlpha,
                    .duration = duration
                };

                this->tooltipTween = std::move(
                    this->tweenSystem->CreateTween(config)
                        .Bind([this](float newValue) {
                            this->currentTooltipAlpha = newValue;

                            if (newValue <= 0.0f) {
                                this->tooltipNode->GetObject<UiVisual>()->SetEnabled(false);
                                this->tooltipNode->GetObject<UiText>()->SetEnabled(false);
                            } else {
                                this->tooltipNode->GetObject<UiVisual>()->SetEnabled(true);
                                this->tooltipNode->GetObject<UiText>()->SetEnabled(true);
                            }
                        })
                );
            } else {
                this->currentTooltipAlpha = shouldBeVisible ? 1.0f : 0.0f;
                this->tooltipNode->GetObject<UiVisual>()->SetEnabled(shouldBeVisible);
                this->tooltipNode->GetObject<UiText>()->SetEnabled(shouldBeVisible);
            }
        }
            if (this->currentTooltipAlpha > 0.0f) {
                float parentAlpha = 1.0f;
                if (auto* parentVisual = this->GetObject<UiVisual>()) {
                    parentAlpha = parentVisual->color.a;
                }

                this->tooltipNode->GetObject<UiVisual>()->color.a = 0.2f * parentAlpha * this->currentTooltipAlpha;
                this->tooltipNode->GetObject<UiText>()->color.a = 1.0f * parentAlpha * this->currentTooltipAlpha;

                glm::vec2 resolution = this->GetScene()->GetGraphics()->GetScreenResolution();
                float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

                glm::vec2 virtualMouse = mousePosition / scaleFactor;

                this->tooltipNode->GetObject<UiLayout>()->offset = {
                    (int)virtualMouse.x - 400 - 100,
                    (int)virtualMouse.y - 50,
                };
            } else {
                this->tooltipNode->GetObject<UiVisual>()->SetEnabled(false);
                this->tooltipNode->GetObject<UiText>()->SetEnabled(false);
            }
    }

    void RenderItems() {
        float sliceAngle = glm::two_pi<float>() / numberOfSlices;
        float midRadius = innerRadius + ((outerRadius - innerRadius) / 2.0f);
        float rotationSpeed = 0.02f;

        for (int i = 0; i < numberOfSlices; i++) {
            auto currentRotation = itemSlots[i].itemNode->LocalTransform().Rotation();
            glm::quat deltaRotation = glm::angleAxis(rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
            itemSlots[i].itemNode->LocalTransform().Rotation() = currentRotation * deltaRotation;

            if (itemSlots[i].uiNode) {
                UiLayout* itemLayout = itemSlots[i].uiNode->GetObject<UiLayout>();
                if (itemLayout) {
                    float angle = (i * sliceAngle) + (sliceAngle / 2.0f);
                    float xOffset = std::cos(angle) * midRadius;
                    float yOffset = std::sin(angle) * midRadius;

                    itemLayout->offset = glm::ivec2(static_cast<int>(xOffset), static_cast<int>(yOffset));
                }
            }
        }
    }

    void SetLayerRecursive(SceneNode* node, uint32_t layer) {
        if (!node) return;
        node->SetLayer(layer);
        for (auto* child : node->GetChildren()) {
            SetLayerRecursive(child, layer);
        }
    }
};
