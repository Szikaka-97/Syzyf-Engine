#pragma once

#include "Scene.h"
#include "game_scripts/crafting/CraftingNodeUtils.h"

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <array>
#include <initializer_list>
#include <string>

#include <spdlog/spdlog.h>

namespace Crafting{
    class BottlingStage{
    public:
        static constexpr int BottleCount = 4;

        int requiredFilledBottles = BottleCount - 1;

        float bottleTravelTime = 4.0f;
        float bottleSpawnDelay = 0.85f;
        float fillWindowRadius = 0.28f;

        glm::vec3 bottleVisualOffset = glm::vec3(0.0f, 0.08f, 0.0f);

        std::string startPointNodeName = "Bottle_Start";
        std::string endPointNodeName = "Bottle_stop";
        std::string pourButtonNodeName = "Knob_One.001";
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

            startPointNode = FindFirstExistingNode({
                startPointNodeName,
                "Bottle_Start",
                "BottleStartPoint"
            });

            endPointNode = FindFirstExistingNode({
                endPointNodeName,
                "Bottle_stop",
                "Bottle_Stop",
                "Bottle_End",
                "BottleEndPoint"
            });

            pourButtonNode = FindFirstExistingNode({
                pourButtonNodeName,
                "Knob_One.001",
                "Knob_One",
                "BottleFillPoint"
            });

            bottlesRootNode = FindFirstExistingNode({
                bottlesRootNodeName,
                "BottlingBottlesRoot"
            });

            for (int i = 0; i < BottleCount; ++i){
                bottleNodes[i] = FindNodeRecursive(rootNode,bottleNodeNames[i]);
                liquidNodes[i] = FindNodeRecursive(rootNode,liquidNodeNames[i]);
            }

            LogMissingRequiredNodes();

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
        SceneNode* endPointNode = nullptr;
        SceneNode* pourButtonNode = nullptr;
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

        SceneNode* FindFirstExistingNode(std::initializer_list<std::string> nodeNames) const{
            for (const std::string& nodeName : nodeNames){
                SceneNode* node = FindNodeRecursive(rootNode,nodeName);

                if (node){
                    return node;
                }
            }

            return nullptr;
        }

        void LogMissingRequiredNodes() const{
            if (!rootNode){
                spdlog::warn("BottlingStage: root node is missing.");
                return;
            }

            if (!startPointNode){
                spdlog::warn("BottlingStage: missing Bottle_Start / BottleStartPoint node.");
            }

            if (!endPointNode){
                spdlog::warn("BottlingStage: missing Bottle_stop / Bottle_Stop / BottleEndPoint node.");
            }

            if (!pourButtonNode){
                spdlog::warn("BottlingStage: missing Knob_One.001 / Knob_One / BottleFillPoint node.");
            }

            if (!bottlesRootNode){
                spdlog::warn("BottlingStage: missing BottlingBottlesRoot node. CreateBottlingStageNodes was probably not called.");
            }

            for (int i = 0; i < BottleCount; ++i){
                if (!bottleNodes[i]){
                    spdlog::warn("BottlingStage: missing bottle visual node {}.",bottleNodeNames[i]);
                }

                if (!liquidNodes[i]){
                    spdlog::warn("BottlingStage: missing liquid visual node {}.",liquidNodeNames[i]);
                }
            }
        }

        bool HasRequiredNodes() const{
            if (!rootNode || !startPointNode || !endPointNode || !pourButtonNode){
                return false;
            }

            if (!bottlesRootNode){
                return false;
            }

            for (int i = 0; i < BottleCount; ++i){
                if (!bottleNodes[i] || !liquidNodes[i]){
                    return false;
                }
            }

            return true;
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

        glm::vec3 GetNodePositionInRootSpace(SceneNode* node) const{
            if (!node || !rootNode){
                return glm::vec3(0.0f);
            }

            glm::vec4 localPosition =
                glm::inverse(rootNode->GlobalTransform().Value()) *
                glm::vec4(node->GlobalTransform().Position().Value(), 1.0f);

            return glm::vec3(localPosition);
        }

        glm::vec3 ConvertRootSpacePositionToNodeParent(SceneNode* node,const glm::vec3& rootSpacePosition) const{
            if (!node || !rootNode || !node->GetParent()){
                return rootSpacePosition;
            }

            glm::vec4 worldPosition =
                rootNode->GlobalTransform().Value() *
                glm::vec4(rootSpacePosition, 1.0f);

            glm::vec4 parentLocalPosition =
                glm::inverse(node->GetParent()->GlobalTransform().Value()) *
                worldPosition;

            return glm::vec3(parentLocalPosition);
        }

        glm::vec3 GetStartPosition() const{
            return GetNodePositionInRootSpace(startPointNode);
        }

        glm::vec3 GetEndPosition() const{
            return GetNodePositionInRootSpace(endPointNode);
        }

        float GetPathLength() const{
            return glm::length(
                GetEndPosition() -
                GetStartPosition()
            );
        }

        float GetProjectedTOnBottlePath(const glm::vec3& position) const{
            glm::vec3 startPosition = GetStartPosition();
            glm::vec3 endPosition = GetEndPosition();
            glm::vec3 path = endPosition - startPosition;

            float pathLengthSquared = glm::dot(path,path);

            if (pathLengthSquared < 0.0001f){
                return 0.5f;
            }

            float t =
                glm::dot(position - startPosition,path) /
                pathLengthSquared;

            return glm::clamp(t,0.0f,1.0f);
        }

        float GetFillT() const{
            return GetProjectedTOnBottlePath(
                GetNodePositionInRootSpace(pourButtonNode)
            );
        }

        glm::vec3 GetFillPosition() const{
            return glm::mix(
                GetStartPosition(),
                GetEndPosition(),
                GetFillT()
            );
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

        void SetBottlePosition(SceneNode* bottleNode,const glm::vec3& rootSpacePosition){
            if (!bottleNode){
                return;
            }

            bottleNode->LocalTransform().Position() =
                ConvertRootSpacePositionToNodeParent(
                    bottleNode,
                    rootSpacePosition
                );
        }

        void ResetBottlePositions(){
            if (!HasRequiredNodes()){
                return;
            }

            for (int i = 0; i < BottleCount; ++i){
                SetBottlePosition(
                    bottleNodes[i],
                    GetStartPosition() + bottleVisualOffset
                );

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
                    SetBottlePosition(
                        bottleNodes[i],
                        GetStartPosition() + bottleVisualOffset
                    );

                    continue;
                }

                SetBottlePosition(
                    bottleNodes[i],
                    GetBottleVisualPositionAt(i)
                );
            }
        }

        void UpdateFillWindowState(){
            int bestBottle = FindBestBottleInFillWindow();

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
            float bestDistance = fillWindowRadius;

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
                    glm::distance(bottlePathPosition,fillPosition);

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