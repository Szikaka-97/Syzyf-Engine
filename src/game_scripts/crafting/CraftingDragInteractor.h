#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputSystem.h"
#include "Scene.h"

#include "game_scripts/crafting/DraggableCraftingItem.h"
#include "game_scripts/crafting/CraftingIngredientReceiver.h"

#include <physics/System.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <vector>

namespace Crafting
{
    class CraftingDragInteractor : public GameObject
    {
    public:
        float interactionDistance = 100.0f;
        glm::vec2 viewportSize = glm::vec2(1024.0f, 576.0f);

        void SetViewportSize(const glm::vec2& size)
        {
            viewportSize = size;
        }

        void Awake()
        {
            spdlog::info("CraftingDragInteractor ready.");
            spdlog::info("Hold LMB on ingredient and drag it with mouse.");
        }

        void Update()
        {
            if (!GetScene() || !GetScene()->Input())
            {
                return;
            }

            if (GetScene()->Input()->ButtonDown(MouseButton::Left))
            {
                TryBeginDrag();
            }

            if (heldItem && GetScene()->Input()->ButtonPressed(MouseButton::Left))
            {
                UpdateDraggedItem();
            }

            if (heldItem && GetScene()->Input()->ButtonUp(MouseButton::Left))
            {
                EndDrag();
            }
        }

    private:
        class DraggableItemRayFilter : public JPH::BodyFilter
        {
        public:
            bool ShouldCollide(const JPH::BodyID& inBodyID) const override
            {
                return true;
            }

            bool ShouldCollideLocked(const JPH::Body& inBody) const override
            {
                auto* object =
                    reinterpret_cast<GameObject*>(inBody.GetUserData());

                if (!object)
                {
                    return false;
                }

                SceneNode* node = object->GetNode();

                while (node)
                {
                    if (node->GetObject<DraggableCraftingItem>())
                    {
                        return true;
                    }

                    node = node->GetParent();
                }

                return false;
            }
        };

        DraggableCraftingItem* heldItem = nullptr;
        float dragPlaneY = 0.0f;
        glm::vec3 grabOffset = glm::vec3(0.0f);

        void TryBeginDrag()
        {
            Camera* camera = FindMainCamera();

            if (!camera)
            {
                spdlog::warn("CraftingDragInteractor: main camera not found.");
                return;
            }

            glm::vec3 rayOrigin;
            glm::vec3 rayDirection;

            if (!BuildMouseRay(camera, rayOrigin, rayDirection))
            {
                spdlog::warn("CraftingDragInteractor: failed to build mouse ray.");
                return;
            }

            glm::vec3 hitPoint;
            DraggableCraftingItem* item =
                RaycastDraggableItem(rayOrigin, rayDirection, hitPoint);

            if (!item)
            {
                spdlog::info("Crafting drag: no draggable item under mouse.");
                return;
            }

            glm::vec3 itemPosition =
                item->GetNode()->GlobalTransform().Position().Value();

            dragPlaneY = hitPoint.y;
            grabOffset = itemPosition - hitPoint;

            heldItem = item;
            heldItem->BeginDrag();

            UpdateDraggedItem();
        }

        void UpdateDraggedItem()
        {
            if (!heldItem)
            {
                return;
            }

            Camera* camera = FindMainCamera();

            if (!camera)
            {
                return;
            }

            glm::vec3 rayOrigin;
            glm::vec3 rayDirection;

            if (!BuildMouseRay(camera, rayOrigin, rayDirection))
            {
                return;
            }

            glm::vec3 pointOnPlane;

            if (!RaycastToHorizontalPlane(rayOrigin, rayDirection, dragPlaneY, pointOnPlane))
            {
                return;
            }

            glm::vec3 targetPosition = pointOnPlane + grabOffset;
            targetPosition.y = dragPlaneY;

            heldItem->DragTo(targetPosition);
        }

        void EndDrag()
        {
            if (!heldItem)
            {
                return;
            }

            DraggableCraftingItem* releasedItem = heldItem;

            releasedItem->EndDrag();

            TryDropIntoIngredientReceiver(releasedItem);

            heldItem = nullptr;
            grabOffset = glm::vec3(0.0f);
            dragPlaneY = 0.0f;
        }

        void TryDropIntoIngredientReceiver(DraggableCraftingItem* item)
        {
            if (!item || !GetScene())
            {
                return;
            }

            CraftingIngredientReceiver* receiver =
                FindReceiverContainingItem(item);

            if (!receiver)
            {
                spdlog::info(
                    "Crafting drag: {} was not released inside Cauldron receiver hitbox.",
                    item->data.displayName
                );

                if (item->returnToStartOnInvalidDrop)
                {
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
        )
        {
            if (!item || !GetScene())
            {
                return nullptr;
            }

            glm::vec3 itemPosition =
                item->GetNode()->GlobalTransform().Position().Value();

            std::vector<CraftingIngredientReceiver*> receivers =
                GetScene()->FindObjectsOfType<CraftingIngredientReceiver>();

            for (auto* receiver : receivers)
            {
                if (!receiver || !receiver->GetNode())
                {
                    continue;
                }

                if (receiver->ContainsWorldPoint(itemPosition))
                {
                    return receiver;
                }
            }

            return nullptr;
        }

        bool BuildMouseRay(Camera* camera, glm::vec3& outOrigin, glm::vec3& outDirection)
        {
            if (!camera || !GetScene() || !GetScene()->Input())
            {
                return false;
            }

            glm::vec2 mousePosition = GetScene()->Input()->GetMousePosition();

            if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f)
            {
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
        )
        {
            if (glm::abs(rayDirection.y) < 0.0001f)
            {
                return false;
            }

            float t = (planeY - rayOrigin.y) / rayDirection.y;

            if (t < 0.0f || t > interactionDistance)
            {
                return false;
            }

            outPoint = rayOrigin + rayDirection * t;
            return true;
        }

        DraggableCraftingItem* RaycastDraggableItem(
            const glm::vec3& origin,
            const glm::vec3& direction,
            glm::vec3& outHitPoint
        )
        {
            auto* physics = GetScene()->GetComponent<Physics::System>();

            if (!physics)
            {
                spdlog::warn("CraftingDragInteractor: Physics::System not found.");
                return nullptr;
            }

            DraggableItemRayFilter draggableFilter;

            Physics::RayCastPayload hit =
                physics->CastRay(
                    origin,
                    direction * interactionDistance,
                    {},
                    {},
                    draggableFilter
                );

            if (!hit.hasHit)
            {
                return nullptr;
            }

            DraggableCraftingItem* item = FindDraggableOnNode(hit.node);

            if (!item)
            {
                return nullptr;
            }

            outHitPoint = hit.position;

            spdlog::info(
                "Crafting drag: raycast selected {}.",
                item->data.displayName
            );

            return item;
        }

        DraggableCraftingItem* FindDraggableOnNode(SceneNode* node)
        {
            SceneNode* current = node;

            while (current)
            {
                if (auto* item = current->GetObject<DraggableCraftingItem>())
                {
                    return item;
                }

                current = current->GetParent();
            }

            return nullptr;
        }

        Camera* FindMainCamera()
        {
            std::vector<Camera*> cameras = GetScene()->FindObjectsOfType<Camera>();

            if (cameras.empty())
            {
                return nullptr;
            }

            return cameras[0];
        }
    };
}