#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"
#include "Graphics.h"

#include "game_scripts/crafting/CraftingInteractable.h"
#include "game_scripts/crafting/DraggableCraftingItem.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"
#include "game_scripts/crafting/CraftingStation.h"
#include "game_scripts/crafting/CraftingNodeUtils.h"

#include <physics/System.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <vector>

namespace Crafting{
    class CraftingDragInteractor : public GameObject{
    public:
        float interactionDistance = 100.0f;
        glm::vec2 viewportSize = glm::vec2(1024.0f, 576.0f);

        void SetViewportSize(const glm::vec2& size){
            viewportSize = size;
        }

        void Awake(){
            spdlog::info("CraftingDragInteractor ready.");
            spdlog::info("CraftingDragInteractor uses physics raycast only.");
        }

        void Update(){
            if (!GetScene() || !GetScene()->Input()){
                return;
            }

            if (GetScene()->Input()->ButtonDown(MouseButton::Left)){
                TryBeginInteraction();
            }

            if (heldItem && GetScene()->Input()->ButtonPressed(MouseButton::Left)){
                UpdateDraggedItem();
            }

            if (heldItem && GetScene()->Input()->ButtonUp(MouseButton::Left)){
                EndDrag();
            }
        }

    private:
        class InteractableRayFilter : public JPH::BodyFilter{
        public:
            explicit InteractableRayFilter(CraftingInteractionMask mask)
                : activeMask(mask){}

            bool ShouldCollide(const JPH::BodyID& inBodyID) const override{
                return activeMask != ToMask(CraftingInteractionType::None);
            }

            bool ShouldCollideLocked(const JPH::Body& inBody) const override{
                auto* object =
                    reinterpret_cast<GameObject*>(inBody.GetUserData());

                if (!object){
                    return false;
                }

                SceneNode* node = object->GetNode();

                while (node){
                    if (!node->IsEnabled()){
                        return false;
                    }

                    if (auto* interactable = node->GetObject<CraftingInteractable>()){
                        return interactable->MatchesMask(activeMask);
                    }

                    node = node->GetParent();
                }

                return false;
            }

        private:
            CraftingInteractionMask activeMask = ToMask(CraftingInteractionType::None);
        };

        struct InteractableHit{
            CraftingInteractable* interactable = nullptr;
            SceneNode* node = nullptr;
            glm::vec3 position = glm::vec3(0.0f);
        };

        DraggableCraftingItem* heldItem = nullptr;
        float dragPlaneY = 0.0f;
        glm::vec3 grabOffset = glm::vec3(0.0f);

        void TryBeginInteraction(){
            CraftingStation* station = FindActiveStation();

            if (!station){
                spdlog::info("Crafting interaction: no active CraftingStation.");
                return;
            }

            CraftingInteractionMask activeMask = station->GetActiveInteractionMask();

            if (activeMask == ToMask(CraftingInteractionType::None)){
                return;
            }

            Camera* camera = FindMainCamera();

            if (!camera){
                spdlog::warn("CraftingDragInteractor: main camera not found.");
                return;
            }

            glm::vec3 rayOrigin;
            glm::vec3 rayDirection;

            if (!BuildMouseRay(camera, rayOrigin, rayDirection)){
                spdlog::warn("CraftingDragInteractor: failed to build mouse ray.");
                return;
            }

            InteractableHit hit;

            if (!RaycastInteractablePhysics(rayOrigin, rayDirection, activeMask, hit)){
                return;
            }

            if (!hit.interactable){
                return;
            }

            switch (hit.interactable->type){
                case CraftingInteractionType::Ingredient:
                    TryBeginDragFromHit(hit);
                    break;

                case CraftingInteractionType::Lid:
                    station->OnLidClicked();
                    break;

                case CraftingInteractionType::Blower:
                    station->OnBlowerClicked();
                    break;

                case CraftingInteractionType::Door:
                    station->OnDoorClicked();
                    break;

                case CraftingInteractionType::Valve:
                    station->OnValveClicked();
                    break;

                case CraftingInteractionType::None:
                default:
                    break;
            }
        }

        void TryBeginDragFromHit(const InteractableHit& hit){
            DraggableCraftingItem* item = FindObjectOnNodeOrParents<DraggableCraftingItem>(hit.node);

            if (!item){
                spdlog::warn("Crafting drag: Ingredient interactable has no DraggableCraftingItem.");
                return;
            }

            glm::vec3 itemPosition =
                item->GetNode()->GlobalTransform().Position().Value();

            dragPlaneY = hit.position.y;
            grabOffset = itemPosition - hit.position;

            heldItem = item;
            heldItem->BeginDrag();

            spdlog::info(
                "Crafting drag: picked interactable ingredient {}.",
                item->data.displayName
            );

            UpdateDraggedItem();
        }

        void UpdateDraggedItem(){
            if (!heldItem){
                return;
            }

            Camera* camera = FindMainCamera();

            if (!camera){
                return;
            }

            glm::vec3 rayOrigin;
            glm::vec3 rayDirection;

            if (!BuildMouseRay(camera, rayOrigin, rayDirection)){
                return;
            }

            glm::vec3 pointOnPlane;

            if (!RaycastToHorizontalPlane(rayOrigin, rayDirection, dragPlaneY, pointOnPlane)){
                return;
            }

            glm::vec3 targetPosition = pointOnPlane + grabOffset;
            targetPosition.y = dragPlaneY;

            heldItem->DragTo(targetPosition);
        }

        void EndDrag(){
            if (!heldItem){
                return;
            }

            DraggableCraftingItem* releasedItem = heldItem;

            releasedItem->EndDrag();

            TryDropIntoIngredientReceiver(releasedItem);

            heldItem = nullptr;
            grabOffset = glm::vec3(0.0f);
            dragPlaneY = 0.0f;
        }

        void TryDropIntoIngredientReceiver(DraggableCraftingItem* item){
            if (!item || !GetScene()){
                return;
            }

            CraftingIngredientReceiver* receiver =
                FindReceiverContainingItem(item);

            if (!receiver){
                spdlog::info(
                    "Crafting drag: {} was not released inside Cauldron receiver hitbox.",
                    item->data.displayName
                );

                if (item->returnToStartOnInvalidDrop){
                    item->ReturnToStart();
                }

                return;
            }

            spdlog::info(
                "Crafting drag: released {} inside ingredient receiver.",
                item->data.displayName
            );

            receiver->OnCollisionEnter(item->GetNode());
        }

        CraftingIngredientReceiver* FindReceiverContainingItem(
            DraggableCraftingItem* item
        ){
            if (!item || !GetScene()){
                return nullptr;
            }

            glm::vec3 itemPosition =
                item->GetNode()->GlobalTransform().Position().Value();

            std::vector<CraftingIngredientReceiver*> receivers =
                GetScene()->FindObjectsOfType<CraftingIngredientReceiver>();

            for (auto* receiver : receivers){
                if (!receiver || !receiver->GetNode()){
                    continue;
                }

                if (receiver->ContainsWorldPoint(itemPosition)){
                    return receiver;
                }
            }

            return nullptr;
        }

        bool RaycastInteractablePhysics(
            const glm::vec3& origin,
            const glm::vec3& direction,
            CraftingInteractionMask activeMask,
            InteractableHit& outHit
        ){
            auto* physics = GetScene()->GetComponent<Physics::System>();

            if (!physics){
                spdlog::warn("CraftingDragInteractor: Physics::System not found.");
                return false;
            }

            InteractableRayFilter interactableFilter(activeMask);

            Physics::RayCastPayload hit =
                physics->CastRay(
                    origin,
                    direction * interactionDistance,
                    {},
                    {},
                    interactableFilter
                );

            if (!hit.hasHit){
                return false;
            }

            CraftingInteractable* interactable =
                FindInteractableOnNode(hit.node, activeMask);

            if (!interactable){
                spdlog::warn("Crafting interaction: physics hit has no matching CraftingInteractable.");
                return false;
            }

            outHit.interactable = interactable;
            outHit.node = hit.node;
            outHit.position = hit.position;

            return true;
        }

        CraftingInteractable* FindInteractableOnNode(
            SceneNode* node,
            CraftingInteractionMask activeMask
        ){
            SceneNode* current = node;

            while (current){
                if (auto* interactable = current->GetObject<CraftingInteractable>()){
                    if (interactable->MatchesMask(activeMask)){
                        return interactable;
                    }
                }

                current = current->GetParent();
            }

            return nullptr;
        }

        bool BuildMouseRay(Camera* camera, glm::vec3& outOrigin, glm::vec3& outDirection){
            if (!camera || !GetScene() || !GetScene()->Input()){
                return false;
            }

            glm::vec2 mousePosition = GetScene()->Input()->GetMousePosition();

            this->viewportSize = GetScene()->GetGraphics()->GetScreenResolution();

            if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f){
                spdlog::warn(
                    "CraftingDragInteractor: invalid viewport size {} x {}.",
                    viewportSize.x,
                    viewportSize.y
                );
                return false;
            }

            float ndcX = (2.0f * mousePosition.x) / viewportSize.x - 1.0f;
            float ndcY = 1.0f - (2.0f * mousePosition.y) / viewportSize.y;

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

        bool RaycastToHorizontalPlane(
            const glm::vec3& rayOrigin,
            const glm::vec3& rayDirection,
            float planeY,
            glm::vec3& outPoint
        ){
            if (glm::abs(rayDirection.y) < 0.0001f){
                return false;
            }

            float t = (planeY - rayOrigin.y) / rayDirection.y;

            if (t < 0.0f || t > interactionDistance){
                return false;
            }

            outPoint = rayOrigin + rayDirection * t;
            return true;
        }

        CraftingStation* FindActiveStation(){
            std::vector<CraftingStation*> stations =
                GetScene()->FindObjectsOfType<CraftingStation>();

            for (auto* station : stations){
                if (station && station->IsActive()){
                    return station;
                }
            }

            return nullptr;
        }

        Camera* FindMainCamera(){
            if (!GetScene()){
                return nullptr;
            }

            if (GetScene()->GetGraphics() && GetScene()->GetGraphics()->GetMainCamera()){
                return GetScene()->GetGraphics()->GetMainCamera();
            }

            SceneNode* cameraNode = GetScene()->FindNode("Camera Node");

            if (cameraNode){
                if (auto* camera = cameraNode->GetObject<Camera>()){
                    return camera;
                }
            }

            std::vector<Camera*> cameras =
                GetScene()->FindObjectsOfType<Camera>();

            if (cameras.empty()){
                return nullptr;
            }

            return cameras[0];
        }
    };
}