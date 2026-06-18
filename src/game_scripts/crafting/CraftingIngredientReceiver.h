#pragma once

#include <GameObject.h>
#include <physics/ICollisionReceiver.h>

#include "DraggableCraftingItem.h"
#include "Cauldron.h"
#include "CraftingNodeUtils.h"
#include <game_scripts/PotionInventory.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Crafting{
    class CraftingIngredientReceiver :
        public GameObject,
        public Physics::ICollisionReceiver{
    public:
          std::vector<IngredientType> insertedIngredients;

          glm::vec3 ingredientConsumeOffset = glm::vec3(0.0f, 0.0f, 0.0f);

          glm::vec3 receiverHalfExtents = glm::vec3(0.7f, 2.0f, 0.7f);

          bool ContainsWorldPoint(const glm::vec3& point) const{
              glm::vec3 receiverPosition =
                  GetNode()->GlobalTransform().Position().Value();

              glm::vec3 difference =
                  point - receiverPosition;

              return
                  std::abs(difference.x) <= receiverHalfExtents.x &&
                  std::abs(difference.y) <= receiverHalfExtents.y &&
                  std::abs(difference.z) <= receiverHalfExtents.z;
          }

        void OnCollisionEnter(SceneNode* otherNode) override{
            auto* item = FindObjectOnNodeOrParents<DraggableCraftingItem>(otherNode);

            if (!item){return;}

            if (item->isDragged){
                return;
            }

            auto* cauldron = FindObjectOnNodeOrParents<Cauldron>(GetNode());

            if (!cauldron){
                return;
            }

            if (item->data.role == IngredientRole::None){
                item->ReturnToStart();
                return;
            }

            SceneNode* itemNode = item->GetNode();

            if (WasAlreadyInserted(itemNode)){return;}

            if (!cauldron->CanAddIngredient(item->data)){
                item->ReturnToStart();
                return;
            }

            std::string inventoryKey = item->inventoryKey;

            if (inventoryKey.empty()){
                inventoryKey = item->data.inventoryKey;
            }

            if (!PotionInventory::ConsumeIngredient(inventoryKey,1)){
                item->ReturnToStart();
                return;
            }

            item->data.inventoryKey = inventoryKey;

            if (!cauldron->AddIngredient(item->data)){
                PotionInventory::RefundIngredient(inventoryKey,1);
                item->ReturnToStart();
                return;
            }

            insertedItemNodes.push_back(itemNode);
            insertedIngredients.push_back(item->data.type);


            glm::vec3 consumePosition =
                GetNode()->GlobalTransform().Position().Value() + ingredientConsumeOffset;

            item->ConsumeAt(consumePosition);
        }

          void OnCollisionExit(SceneNode* otherNode) override{}

          void Clear(){
                insertedIngredients.clear();
                insertedItemNodes.clear();
          }

    private:
        std::vector<SceneNode*> insertedItemNodes;

        bool WasAlreadyInserted(SceneNode* node) const{
            return std::find(
                insertedItemNodes.begin(),
                insertedItemNodes.end(),
                node
            ) != insertedItemNodes.end();
        }

    };
}