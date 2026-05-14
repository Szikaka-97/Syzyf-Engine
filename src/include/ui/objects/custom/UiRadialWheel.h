#pragma once

#include "Camera.h"
#include "Debug.h"
#include "Framebuffer.h"
#include "GameObject.h"
#include "GltfImporter.h"
#include "InputSystem.h"
#include "Material.h"
#include "MeshRenderer.h"
#include "Viewport.h"
#include "ui/Wheel.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiVisual.h"

#include <cmath>
#include <imgui.h>
#include <filesystem>

namespace fs = std::filesystem;

class UiRadialWheel : public GameObject, public ImGuiDrawable {
public:
    int numberOfSlices = 5;

    float innerRadius = 25.0f;
    float outerRadius = 300.0f;

    std::unique_ptr<Material> material;

private:
    struct ItemSlot {
        std::unique_ptr<Viewport> viewport;
        SceneNode* cameraNode = nullptr;
        SceneNode* itemNode = nullptr;
        SceneNode* uiNode = nullptr;
    };

    std::vector<ItemSlot> itemSlots;

    int hoveredSlice = -1;

public:
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
            visual->color.a = 0.0f;

            UiLayout* layout = slot.uiNode->AddObject<UiLayout>();
            layout->size = glm::ivec2(128, 128);
            layout->anchorPoint = AnchorPoint::Center;
            layout->zIndex = 1;

            itemSlots.push_back(std::move(slot));
        }
    }

    void Update() {
        InputSystem* input = GetScene()->Input();
        auto* layout = GetNode()->GetObject<UiLayout>();

        if (!material || !input || !layout) return;

        glm::vec3 globalPosition = GetNode()->GlobalTransform().Position();
        glm::vec2 wheelCenter = glm::vec2(globalPosition.x, globalPosition.y);

        glm::vec2 mousePosition = input->GetMousePosition();
        glm::vec2 relativeMouse = mousePosition - wheelCenter;
        float distance = glm::length(relativeMouse);

        if (distance >= innerRadius && distance <= outerRadius) {
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

        material->SetValue("hoveredSlice", (unsigned int)hoveredSlice);
        material->SetValue("numberOfSlices", (unsigned int)numberOfSlices);

        this->RenderItems();
    }

    void DrawImGui() {
    }

private:
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
