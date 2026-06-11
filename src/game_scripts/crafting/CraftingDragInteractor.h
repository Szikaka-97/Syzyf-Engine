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
#include "MeshRenderer.h"

#include <physics/System.h>
#include <TimeSystem.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <utility>
#include <cmath>
#include <limits>

namespace Crafting{
    class CraftingDragInteractor : public GameObject{
    public:
        float interactionDistance = 100.0f;

        void Update(){
            if (!GetScene() || !GetScene()->Input()){
                return;
            }

            blinkTime += Time::Delta();

            UpdateHoverHighlight();

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
                if ((inBody.GetCollisionGroup().GetGroupID() & CraftingInteractionCollisionMask) == 0){
                    return false;
                }

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
        float blinkTime = 0.0f;
        glm::vec3 grabOffset = glm::vec3(0.0f);
        std::vector<std::pair<MeshRenderer*, uint8_t>> highlightedRenderers;

        void ClearHoverHighlight(){
            for (auto& highlightedRenderer : highlightedRenderers){
                if (highlightedRenderer.first){
                    highlightedRenderer.first->maskFlags = highlightedRenderer.second;
                }
            }

            highlightedRenderers.clear();
        }

        bool IsRendererAlreadyHighlighted(MeshRenderer* renderer) const{
            for (const auto& highlightedRenderer : highlightedRenderers){
                if (highlightedRenderer.first == renderer){
                    return true;
                }
            }

            return false;
        }

        void CollectMeshRenderersRecursive(SceneNode* node, std::vector<MeshRenderer*>& renderers){
            if (!node){
                return;
            }

            if (auto* renderer = node->GetObject<MeshRenderer>()){
                renderers.push_back(renderer);
            }

            for (SceneNode* child : node->GetChildren()){
                CollectMeshRenderersRecursive(child, renderers);
            }
        }

        void AddOutlineForNode(SceneNode* node){
            if (!node){
                return;
            }

            std::vector<MeshRenderer*> renderers;
            CollectMeshRenderersRecursive(node, renderers);

            if (renderers.empty() && node->GetParent()){
                CollectMeshRenderersRecursive(node->GetParent(), renderers);
            }

            for (auto* renderer : renderers){
                if (!renderer){
                    continue;
                }

                if (IsRendererAlreadyHighlighted(renderer)){
                    continue;
                }

                highlightedRenderers.push_back({renderer, renderer->maskFlags});
                renderer->maskFlags = renderer->maskFlags | MaskEffectBits::Jfa;
            }
        }

        void SetHoverHighlight(SceneNode* node){
            ClearHoverHighlight();
            AddOutlineForNode(node);
        }

        void CollectBlinkingInteractableNodesRecursive(
            SceneNode* node,
            CraftingInteractionMask activeMask,
            std::vector<SceneNode*>& interactableNodes
        ){
            if (!node || !node->IsEnabled()){
                return;
            }

            if (auto* interactable = node->GetObject<CraftingInteractable>()){
                if (interactable->MatchesMask(activeMask) && interactable->ShouldBlinkOutline()){
                    interactableNodes.push_back(node);
                }
            }

            for (SceneNode* child : node->GetChildren()){
                CollectBlinkingInteractableNodesRecursive(
                    child,
                    activeMask,
                    interactableNodes
                );
            }
        }

        void SetBlinkHighlight(CraftingStation* station, CraftingInteractionMask activeMask){
            ClearHoverHighlight();

            float blink =
                0.5f + 0.5f * std::sin(blinkTime * 3.0f);

            if (blink < 0.35f){
                return;
            }

            std::vector<SceneNode*> interactableNodes;

            CollectBlinkingInteractableNodesRecursive(
                station->GetNode(),
                activeMask,
                interactableNodes
            );

            for (SceneNode* interactableNode : interactableNodes){
                AddOutlineForNode(interactableNode);
            }
        }

        void UpdateHoverHighlight(){
            if (heldItem){
                ClearHoverHighlight();
                return;
            }

            CraftingStation* station = FindActiveStation();

            if (!station){
                ClearHoverHighlight();
                return;
            }

            CraftingInteractionMask activeMask = station->GetActiveInteractionMask();

            if (activeMask == ToMask(CraftingInteractionType::None)){
                ClearHoverHighlight();
                return;
            }

            Camera* camera = FindMainCamera();

            if (!camera){
                ClearHoverHighlight();
                return;
            }

            glm::vec3 rayOrigin;
            glm::vec3 rayDirection;

            if (!BuildMouseRay(camera, rayOrigin, rayDirection)){
                SetBlinkHighlight(station, activeMask);
                return;
            }

            InteractableHit hit;

            if (!RaycastInteractablePhysics(rayOrigin, rayDirection, activeMask, hit) &&
                !RaycastUiButtonScreenFallback(station, camera, activeMask, hit)){
                SetBlinkHighlight(station, activeMask);
                return;
            }

            if (!hit.interactable || !hit.interactable->GetNode()){
                SetBlinkHighlight(station, activeMask);
                return;
            }

            SetHoverHighlight(hit.interactable->GetNode());
        }

        void TryBeginInteraction(){
            CraftingStation* station = FindActiveStation();

            if (!station){
                return;
            }

            CraftingInteractionMask activeMask = station->GetActiveInteractionMask();

            if (activeMask == ToMask(CraftingInteractionType::None)){
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

            InteractableHit hit;

            if (!RaycastInteractablePhysics(rayOrigin, rayDirection, activeMask, hit) &&
                !RaycastUiButtonScreenFallback(station, camera, activeMask, hit)){
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

                case CraftingInteractionType::UiBack:
                    station->OnUiBackClicked();
                    break;

                case CraftingInteractionType::UiInfo:
                    station->OnUiInfoClicked();
                    break;

                case CraftingInteractionType::UiNext:
                    station->OnUiNextClicked();
                    break;

                case CraftingInteractionType::InventoryNextPage:
                    station->OnInventoryNextPageClicked();
                    break;

                case CraftingInteractionType::InventoryPreviousPage:
                    station->OnInventoryPreviousPageClicked();
                    break;

                case CraftingInteractionType::None:
                default:
                    break;
            }
        }

        void TryBeginDragFromHit(const InteractableHit& hit){
            DraggableCraftingItem* item = FindObjectOnNodeOrParents<DraggableCraftingItem>(hit.node);

            if (!item){
                return;
            }

            glm::vec3 itemPosition =
                item->GetNode()->GlobalTransform().Position().Value();

            dragPlaneY = hit.position.y;
            grabOffset = glm::vec3(0.0f);

            heldItem = item;
            heldItem->BeginDrag();

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
                if (item->returnToStartOnInvalidDrop){
                    item->ReturnToStart();
                }

                return;
            }

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


        bool IsUiScreenFallbackType(CraftingInteractionType type) const{
            return
                type == CraftingInteractionType::UiBack ||
                type == CraftingInteractionType::UiInfo ||
                type == CraftingInteractionType::UiNext ||
                type == CraftingInteractionType::InventoryNextPage ||
                type == CraftingInteractionType::InventoryPreviousPage;
        }

        bool IsNodeAndParentsEnabled(SceneNode* node) const{
            SceneNode* current = node;

            while (current){
                if (!current->IsEnabled()){
                    return false;
                }

                current = current->GetParent();
            }

            return true;
        }

        void CollectUiButtonInteractablesRecursive(
            SceneNode* node,
            CraftingInteractionMask activeMask,
            std::vector<CraftingInteractable*>& interactables
        ){
            if (!node || !IsNodeAndParentsEnabled(node)){
                return;
            }

            if (auto* interactable = node->GetObject<CraftingInteractable>()){
                if (interactable->MatchesMask(activeMask) &&
                    IsUiScreenFallbackType(interactable->type)){
                    interactables.push_back(interactable);
                }
            }

            for (SceneNode* child : node->GetChildren()){
                CollectUiButtonInteractablesRecursive(child, activeMask, interactables);
            }
        }

        bool ProjectWorldToScreen(
            Camera* camera,
            const glm::vec3& worldPosition,
            glm::vec2& outScreenPosition
        ) const{
            if (!camera || !GetScene() || !GetScene()->GetGraphics()){
                return false;
            }

            glm::vec2 screenSize = GetScene()->GetGraphics()->GetScreenResolution();

            if (screenSize.x <= 0.0f || screenSize.y <= 0.0f){
                return false;
            }

            glm::vec4 clipPosition =
                camera->ProjectionMatrix() *
                camera->ViewMatrix() *
                glm::vec4(worldPosition, 1.0f);

            if (clipPosition.w <= 0.0001f){
                return false;
            }

            glm::vec3 ndc = glm::vec3(clipPosition) / clipPosition.w;

            if (ndc.x < -1.15f || ndc.x > 1.15f ||
                ndc.y < -1.15f || ndc.y > 1.15f ||
                ndc.z < -1.15f || ndc.z > 1.15f){
                return false;
            }

            outScreenPosition.x = (ndc.x * 0.5f + 0.5f) * screenSize.x;
            outScreenPosition.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * screenSize.y;
            return true;
        }

        bool RaycastUiButtonScreenFallback(
            CraftingStation* station,
            Camera* camera,
            CraftingInteractionMask activeMask,
            InteractableHit& outHit
        ){
            if (!station || !camera || !GetScene() || !GetScene()->Input()){
                return false;
            }

            std::vector<CraftingInteractable*> interactables;

            CollectUiButtonInteractablesRecursive(
                station->GetNode(),
                activeMask,
                interactables
            );

            if (interactables.empty()){
                return false;
            }

            glm::vec2 mousePosition = GetScene()->Input()->GetMousePosition();
            CraftingInteractable* bestInteractable = nullptr;
            glm::vec3 bestPosition = glm::vec3(0.0f);
            float bestDistanceSq = std::numeric_limits<float>::max();
            const float maxDistance = 80.0f;
            const float maxDistanceSq = maxDistance * maxDistance;

            for (auto* interactable : interactables){
                if (!interactable || !interactable->GetNode()){
                    continue;
                }

                glm::vec3 worldPosition =
                    interactable->GetNode()->GlobalTransform().Position().Value();
                glm::vec2 screenPosition;

                if (!ProjectWorldToScreen(camera, worldPosition, screenPosition)){
                    continue;
                }

                glm::vec2 delta = screenPosition - mousePosition;
                float distanceSq = glm::dot(delta, delta);

                if (distanceSq > maxDistanceSq || distanceSq >= bestDistanceSq){
                    continue;
                }

                bestDistanceSq = distanceSq;
                bestInteractable = interactable;
                bestPosition = worldPosition;
            }

            if (!bestInteractable){
                return false;
            }

            outHit.interactable = bestInteractable;
            outHit.node = bestInteractable->GetNode();
            outHit.position = bestPosition;
            return true;
        }

        bool RaycastInteractablePhysics(
            const glm::vec3& origin,
            const glm::vec3& direction,
            CraftingInteractionMask activeMask,
            InteractableHit& outHit
        ){
            auto* physics = GetScene()->GetComponent<Physics::System>();

            if (!physics){
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
            if (!camera || !GetScene() || !GetScene()->Input() || !GetScene()->GetGraphics()){
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
