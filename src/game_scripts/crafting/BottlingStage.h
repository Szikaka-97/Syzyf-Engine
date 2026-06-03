#pragma once

#include "Scene.h"
#include "game_scripts/crafting/CraftingNodeUtils.h"

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <array>
#include <string>

namespace Crafting{
    class BottlingStage{
    public:
        static constexpr int BottleCount = 4;

        int requiredFilledBottles = BottleCount;

        float bottleTravelTime = 4.0f;
        float bottleSpawnDelay = 0.85f;
        float fillZoneRadius = 0.28f;

        glm::vec3 bottleVisualOffset = glm::vec3(0.0f, 0.22f, 0.0f);

        std::string startPointNodeName = "BottleStartPoint";
        std::string fillPointNodeName = "BottleFillPoint";
        std::string endPointNodeName = "BottleEndPoint";
        std::string bottlesRootNodeName = "BottlingBottlesRoot";

        std::array<std::string,BottleCount> bottleNodeNames = {
            "BottlingBottle_01",
            "BottlingBottle_02",
            "BottlingBottle_03",
            "BottlingBottle_04"
        };

        std::array<std::string,BottleCount> liquidNodeNames = {
            "BottlingBottle_01_Liquid",
            "BottlingBottle_02_Liquid",
            "BottlingBottle_03_Liquid",
            "BottlingBottle_04_Liquid"
        };

        void CacheNodes(SceneNode* stationRootNode){
            rootNode = stationRootNode;

            startPointNode = FindNodeRecursive(rootNode,startPointNodeName);
            fillPointNode = FindNodeRecursive(rootNode,fillPointNodeName);
            endPointNode = FindNodeRecursive(rootNode,endPointNodeName);
            bottlesRootNode = FindNodeRecursive(rootNode,bottlesRootNodeName);

            for (int i = 0; i < BottleCount; ++i){
                bottleNodes[i] = FindNodeRecursive(rootNode,bottleNodeNames[i]);
                liquidNodes[i] = FindNodeRecursive(rootNode,liquidNodeNames[i]);
            }

            if (!startPointNode){
            }

            if (!fillPointNode){
            }

            if (!endPointNode){
            }

            if (!bottlesRootNode){
            }

            for (int i = 0; i < BottleCount; ++i){
                if (!bottleNodes[i]){
                }

                if (!liquidNodes[i]){
                }
            }

            SetVisualsEnabled(false);
        }

        void Reset(){
            isActive = false;
            isFinished = false;
            conveyorStarted = false;

            stageTimer = 0.0f;
            filledBottles = 0;
            missedBottles = 0;
            lastBottleInFillWindow = -1;

            for (int i = 0; i < BottleCount; ++i){
                bottleFilled[i] = false;
                bottleFinished[i] = false;
            }

            SetVisualsEnabled(false);
            SetBottlesEnabled(false);

            if (HasRequiredNodes()){
                ResetBottlePositions();
            }
        }

        void Start(){
            isActive = true;
            isFinished = false;
            conveyorStarted = false;

            stageTimer = 0.0f;
            filledBottles = 0;
            missedBottles = 0;
            lastBottleInFillWindow = -1;

            for (int i = 0; i < BottleCount; ++i){
                bottleFilled[i] = false;
                bottleFinished[i] = false;
            }

            SetVisualsEnabled(true);
            ResetBottlePositions();
            SetBottlesEnabled(false);

        }

        void Stop(){
            isActive = false;
            conveyorStarted = false;

            SetBottlesEnabled(false);
            SetVisualsEnabled(false);
        }

        bool Update(float deltaTime){
            if (!isActive || isFinished){
                return isFinished;
            }

            if (!HasRequiredNodes()){
                Finish();
                return true;
            }

            if (!conveyorStarted){
                return false;
            }

            stageTimer += deltaTime;

            UpdateBottlePositions();
            UpdateFillWindowState();
            UpdateFinishedBottles();

            if (AllBottlesFinished()){
                Finish();
            }

            return isFinished;
        }

        void TryFillCurrentBottle(){
            if (!isActive || isFinished){
                return;
            }

            if (!conveyorStarted){
                StartConveyor();
                return;
            }

            int bottleIndex = FindBestBottleInFillWindow();

            if (bottleIndex < 0){
                return;
            }

            if (bottleFilled[bottleIndex]){
                return;
            }

            bottleFilled[bottleIndex] = true;
            ++filledBottles;

            if (liquidNodes[bottleIndex]){
                liquidNodes[bottleIndex]->SetEnabled(true);
            }

        }

        bool IsFinished() const{
            return isFinished;
        }

        int GetFilledBottles() const{
            return filledBottles;
        }

        int GetMissedBottles() const{
            return missedBottles;
        }

        int GetRequiredFilledBottles() const{
            return GetClampedRequiredFilledBottles();
        }

        bool HasEnoughFilledBottles() const{
            return filledBottles >= GetClampedRequiredFilledBottles();
        }

    private:
        SceneNode* rootNode = nullptr;
        SceneNode* startPointNode = nullptr;
        SceneNode* fillPointNode = nullptr;
        SceneNode* endPointNode = nullptr;
        SceneNode* bottlesRootNode = nullptr;

        std::array<SceneNode*,BottleCount> bottleNodes = {};
        std::array<SceneNode*,BottleCount> liquidNodes = {};

        std::array<bool,BottleCount> bottleFilled = {};
        std::array<bool,BottleCount> bottleFinished = {};

        bool isActive = false;
        bool isFinished = false;
        bool conveyorStarted = false;

        int filledBottles = 0;
        int missedBottles = 0;
        int lastBottleInFillWindow = -1;

        float stageTimer = 0.0f;

        bool HasRequiredNodes() const{
            return startPointNode && fillPointNode && endPointNode;
        }

        int GetClampedRequiredFilledBottles() const{
            if (requiredFilledBottles < 1){
                return 1;
            }

            if (requiredFilledBottles > BottleCount){
                return BottleCount;
            }

            return requiredFilledBottles;
        }

        void StartConveyor(){
            if (conveyorStarted){
                return;
            }

            conveyorStarted = true;
            stageTimer = 0.0f;
            lastBottleInFillWindow = -1;

            for (int i = 0; i < BottleCount; ++i){
                bottleFilled[i] = false;
                bottleFinished[i] = false;
            }

            ResetBottlePositions();
            SetBottlesEnabled(true);

        }

        void Finish(){
            if (isFinished){
                return;
            }

            isFinished = true;
            isActive = false;
            conveyorStarted = false;

        }

        glm::vec3 GetStartPosition() const{
            return startPointNode->LocalTransform().Position().Value();
        }

        glm::vec3 GetFillPosition() const{
            return fillPointNode->LocalTransform().Position().Value();
        }

        glm::vec3 GetEndPosition() const{
            return endPointNode->LocalTransform().Position().Value();
        }

        float GetBottleElapsedTime(int bottleIndex) const{
            return stageTimer - bottleSpawnDelay * static_cast<float>(bottleIndex);
        }

        float GetBottleT(int bottleIndex) const{
            float elapsed = GetBottleElapsedTime(bottleIndex);

            if (elapsed <= 0.0f){
                return 0.0f;
            }

            if (bottleTravelTime <= 0.001f){
                return 1.0f;
            }

            return glm::clamp(elapsed / bottleTravelTime, 0.0f, 1.0f);
        }

        glm::vec3 GetBottlePathPositionAt(int bottleIndex) const{
            return glm::mix(
                GetStartPosition(),
                GetEndPosition(),
                GetBottleT(bottleIndex)
            );
        }

        glm::vec3 GetBottleVisualPositionAt(int bottleIndex) const{
            return GetBottlePathPositionAt(bottleIndex) + bottleVisualOffset;
        }

        void ResetBottlePositions(){
            if (!HasRequiredNodes()){
                return;
            }

            for (int i = 0; i < BottleCount; ++i){
                if (bottleNodes[i]){
                    bottleNodes[i]->LocalTransform().Position() =
                        GetStartPosition() + bottleVisualOffset;
                }

                if (liquidNodes[i]){
                    liquidNodes[i]->SetEnabled(false);
                }
            }
        }

        void UpdateBottlePositions(){
            for (int i = 0; i < BottleCount; ++i){
                if (!bottleNodes[i] || bottleFinished[i]){
                    continue;
                }

                if (GetBottleElapsedTime(i) < 0.0f){
                    bottleNodes[i]->LocalTransform().Position() =
                        GetStartPosition() + bottleVisualOffset;

                    continue;
                }

                bottleNodes[i]->LocalTransform().Position() =
                    GetBottleVisualPositionAt(i);
            }
        }

        void UpdateFillWindowState(){
            int bestBottle = FindBestBottleInFillWindow();

            if (bestBottle >= 0 && bestBottle != lastBottleInFillWindow){
            }

            lastBottleInFillWindow = bestBottle;
        }

        void UpdateFinishedBottles(){
            for (int i = 0; i < BottleCount; ++i){
                if (bottleFinished[i]){
                    continue;
                }

                if (GetBottleElapsedTime(i) < bottleTravelTime){
                    continue;
                }

                bottleFinished[i] = true;

                if (!bottleFilled[i]){
                    ++missedBottles;

                }
            }
        }

        bool AllBottlesFinished() const{
            for (int i = 0; i < BottleCount; ++i){
                if (!bottleFinished[i]){
                    return false;
                }
            }

            return true;
        }

        int FindBestBottleInFillWindow() const{
            int bestIndex = -1;
            float bestDistance = fillZoneRadius;

            glm::vec3 fillPosition = GetFillPosition();

            for (int i = 0; i < BottleCount; ++i){
                if (!bottleNodes[i]){
                    continue;
                }

                if (bottleFinished[i] || bottleFilled[i]){
                    continue;
                }

                if (GetBottleElapsedTime(i) < 0.0f){
                    continue;
                }

                glm::vec3 bottlePathPosition =
                    GetBottlePathPositionAt(i);

                float distance =
                    glm::distance(bottlePathPosition, fillPosition);

                if (distance <= bestDistance){
                    bestDistance = distance;
                    bestIndex = i;
                }
            }

            return bestIndex;
        }

        void SetVisualsEnabled(bool enabled){
            if (bottlesRootNode){
                bottlesRootNode->SetEnabled(enabled);
            }
        }

        void SetBottlesEnabled(bool enabled){
            for (int i = 0; i < BottleCount; ++i){
                if (bottleNodes[i]){
                    bottleNodes[i]->SetEnabled(enabled);
                }
            }
        }

    };
}