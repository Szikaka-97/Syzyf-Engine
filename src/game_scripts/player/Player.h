#pragma once

#include <GameObject.h>
#include <glm/ext/matrix_projection.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <Jolt/Jolt.h>
#include "Jolt/Physics/Body/BodyFilter.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "Surface.h"
#include "game_scripts/player/PickableItemSystem.h"
#include "InputSystem.h"
#include "physics/LayerMaskFilter.h"
#include "physics/System.h"
#include "Graphics.h"

class Player : public GameObject {
private: 
    int m_RoomID;
    std::vector<Surface*> rooms;

    // Item interaction stuff
    float itemHighlightRadius = 2.0f;
    PickableItem* highlightedItem = nullptr;
    //  Cached systems
    PickableItemSystem* pickableItemSystem = nullptr;
    Physics::System* physics = nullptr;
public:
    Player() : m_RoomID(0) {
    }

    void Awake() {
        rooms = GetScene()->FindObjectsOfType<Surface>();  
        spdlog::info("Player found {} rooms in the scene", rooms.size());
    }

    void Update() {
        if (HasRoomChanged()) {
            CheckPosition();
        }

        this->HandleItemInteraction();
    }

private:
    // Highlights the item hovered by a cursor, if no item is found falls back to finding the closest item to the Player
    //  pressing a button picks the item up
    void HandleItemInteraction() {
        if (!this->pickableItemSystem) {
            this->pickableItemSystem = this->GetScene()->GetComponent<PickableItemSystem>();
            if (!this->pickableItemSystem) return;
        }

        PickableItem* newItem = nullptr;

        // Mouse cursor raycast
        if (!this->physics) {
            this->physics = this->GetScene()->GetComponent<Physics::System>();
        } else if (auto* camera = this->GetScene()->GetGraphics()->GetMainCamera()) {
            auto* input = this->GetScene()->Input();
            auto* graphics = this->GetScene()->GetGraphics();

            glm::vec2 mousePosition = input->GetMousePosition();
            glm::vec2 screenSize = graphics->GetScreenResolution();

            glm::vec4 viewport(0.0f, 0.0f, screenSize.x, screenSize.y);

            float windowY = screenSize.y - mousePosition.y;

            glm::vec3 windowNear(mousePosition.x, windowY, 0.0f);
            glm::vec3 windowFar(mousePosition.x, windowY, 1.0f);

            glm::mat4 view = camera->ViewMatrix();
            glm::mat4 proj = camera->ProjectionMatrix();

            glm::vec3 rayOrigin = glm::unProject(windowNear, view, proj, viewport);
            glm::vec3 rayTarget = glm::unProject(windowFar, view, proj, viewport);

            glm::vec3 rayDirection = glm::normalize(rayTarget - rayOrigin) * 100.0f;

            // Includes only the items (layer 2)
            Physics::LayerMaskFilter layerFilter({2}, true);

            Physics::RayCastPayload hit = this->physics->CastRay(rayOrigin, rayDirection, {}, {}, layerFilter);

            if (hit.hasHit && hit.node) {
                newItem = hit.node->GetObject<PickableItem>();
            }
        }

        // Closest item fallback
        if (newItem == nullptr) {
            newItem = pickableItemSystem->GetClosestItem(this->GlobalTransform().Position().Value(), this->itemHighlightRadius);
        }

        // Highlighting logic
        if (newItem != this->highlightedItem) {
            if (this->highlightedItem) {
                if (auto* renderer = this->highlightedItem->GetObject<MeshRenderer>()) {
                    renderer->maskFlags &= ~MaskEffectBits::Jfa;
                }
            }
            if (newItem != nullptr) {
                if (auto* renderer = newItem->GetObject<MeshRenderer>()) {
                    renderer->maskFlags |= MaskEffectBits::Jfa;
                }
            }
            this->highlightedItem = newItem;
        }

        // On interact
        if (this->GetScene()->Input()->KeyDown(Key::E) && this->highlightedItem != nullptr) {
            this->highlightedItem->OnPickUp();
            delete this->highlightedItem->GetNode();
            this->highlightedItem = nullptr;
        }
    }

    bool HasRoomChanged() {
        for (auto* room : rooms) {
            if (room->GetID() == m_RoomID) {
                if (!room->ContainsPoint(this->GlobalTransform().Position(), 0.2f)) {
                    room->InformExit();
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    void CheckPosition() {
        for (auto* room : rooms) {
            if (room->GetID() == m_RoomID && room->ContainsPoint(this->GlobalTransform().Position(), 0.0f))
                return; 
        }
        for (auto* room : rooms) {
            if (room->ContainsPoint(this->GlobalTransform().Position(), 0.0f)) {
                m_RoomID = room->GetID();
                room->InformEnter();
                return;
            }
        }
    }
};
