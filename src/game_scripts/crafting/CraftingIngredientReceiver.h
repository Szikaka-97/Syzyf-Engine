#pragma once

#include <GameObject.h>
#include <physics/ICollisionReceiver.h>

#include "DraggableCraftingItem.h"

#include <algorithm>
#include <vector>

#include <spdlog/spdlog.h>

namespace Crafting{
    class CraftingIngredientReceiver :
        public GameObject,
        public Physics::ICollisionReceiver{
    public:
          std::vector<IngredientType> insertedIngredients;

          void OnCollisionEnter(SceneNode* otherNode) override{
                auto* item =
                    FindDraggableOnNode(otherNode);

                if (!item){return;}

                if (item->data.type == IngredientType::None){
                      spdlog::warn("CraftingIngredientReceiver: ignored ingredient with type None.");
                      return;
                }

                SceneNode* itemNode = item->GetNode();

                if (WasAlreadyInserted(itemNode)){
                    return;
                }

                insertedItemNodes.push_back(itemNode);
                insertedIngredients.push_back(item->data.type);

                spdlog::info("Inserted ingredient: {}",item->data.displayName);

                item->Consume();
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

        DraggableCraftingItem* FindDraggableOnNode(SceneNode* node){
            SceneNode* current = node;

            while (current){
                if (auto* item = current->GetObject<DraggableCraftingItem>()){
                    return item;
                }

                current = current->GetParent();
            }

            return nullptr;
        }
    };
}