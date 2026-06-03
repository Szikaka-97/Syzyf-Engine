#pragma once

#include "Scene.h"

#include <physics/Body.h>

#include <string>
#include <vector>

namespace Crafting{
    inline SceneNode* FindNodeRecursive(SceneNode* rootNode, const std::string& targetName){
        if (!rootNode){
            return nullptr;
        }

        if (rootNode->GetName() == targetName){
            return rootNode;
        }

        for (SceneNode* child : rootNode->GetChildren()){
            if (SceneNode* foundNode = FindNodeRecursive(child, targetName)){
                return foundNode;
            }
        }

        return nullptr;
    }

    template<typename TObject>
    inline TObject* FindObjectOnNodeOrParents(SceneNode* node){
        SceneNode* current = node;

        while (current){
            if (auto* object = current->GetObject<TObject>()){
                return object;
            }

            current = current->GetParent();
        }

        return nullptr;
    }

    template<typename TObject>
    inline void CollectObjectsRecursive(SceneNode* rootNode, std::vector<TObject*>& objects){
        if (!rootNode){
            return;
        }

        if (auto* object = rootNode->GetObject<TObject>()){
            objects.push_back(object);
        }

        for (SceneNode* child : rootNode->GetChildren()){
            CollectObjectsRecursive<TObject>(child, objects);
        }
    }

    inline bool IsNodeChildOf(SceneNode* node, SceneNode* expectedParent){
        SceneNode* current = node;

        while (current){
            if (current == expectedParent){
                return true;
            }

            current = current->GetParent();
        }

        return false;
    }

    inline void SyncPhysicsBodyToNode(SceneNode* node){
        if (!node || !node->EnabledSelf()){
            return;
        }

        auto* body = node->GetObject<Physics::Body>();

        if (!body){
            return;
        }

        body->SetPosition(node->GlobalTransform().Position().Value());
        body->SetRotation(node->GlobalTransform().Rotation().Value());
    }
}
