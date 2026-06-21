#pragma once

#include "game_scripts/DungeonGenerator.h"
#include "game_scripts/PlayerController.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiVisual.h"
#include "ui/widgets/wheel/UiWheel.h"

class UiMinimap : public GameObject {
public:
    DungeonGenerator* dungeonGenerator = nullptr;
    SceneNode* playerIndicatorNode = nullptr;

    Texture2D* roomTexture = nullptr;

    float mapScale = 40.0f;

    struct MapRoom {
        glm::vec2 gridPosition;
        SceneNode* node;
        bool visited = false;
    };
    
    std::vector<MapRoom> mapRooms;

    void Initialize(DungeonGenerator* generator) {
        this->dungeonGenerator = generator;
        Scene* scene = GetScene();
        SceneNode* root = GetNode();

        if (!dungeonGenerator) return;

        Texture2D* texture = GetScene()->Resources()->Get<Texture2D>("./res/textures/ui/2d/info_plane.png", Texture2D::ColorTextureRGBA);
        this->roomTexture = GetScene()->Resources()->Get<Texture2D>("./res/textures/ui/2d/slot.png", Texture2D::ColorTextureRGBA);

        SceneNode* bgNode = scene->GetOrCreateNode(root, "Minimap Background");
        bgNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(500, 500),
            glm::ivec2(0, 0),
            0,
            AnchorPoint::Center
        );

        auto* bgVisual = bgNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f), texture);
        bgNode->AddObjectIfMissing<WheelTag>();
        bgVisual->SetEnabled(false);

        playerIndicatorNode = scene->GetOrCreateNode(root, "MapPlayerIndicator");
        playerIndicatorNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(mapScale * 0.4f, mapScale * 0.4f),
            glm::ivec2(0, 0),
            2,
            AnchorPoint::Center
        );

        auto* indicatorVisual = playerIndicatorNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f, 0.2f, 0.2f, 1.0f), texture);
        indicatorVisual->SetEnabled(false);
        playerIndicatorNode->AddObjectIfMissing<WheelTag>();
    }

    void Update() {
        if (!dungeonGenerator || !playerIndicatorNode) return;

        const auto& genRooms = dungeonGenerator->GetRooms();
        if (mapRooms.size() < genRooms.size()) {
            Scene* scene = GetScene();
            SceneNode* root = GetNode();

            for (std::size_t i = mapRooms.size(); i < genRooms.size(); i++) {
                const auto& room = genRooms[i];
                SceneNode* roomUINode = scene->GetOrCreateNode(root, "MapRoom_" + std::to_string(i));

                glm::ivec2 offset(
                    static_cast<int>(room.position.y * mapScale),
                    static_cast<int>(room.position.x * mapScale)
                );

                roomUINode->AddObjectIfMissing<UiLayout>(
                    glm::uvec2(mapScale - 4, mapScale - 4),
                    offset,
                    1,
                    AnchorPoint::Center
                );

                roomUINode->AddObjectIfMissing<UiVisual>(glm::vec4(0.5f, 0.5f, 0.5f, 0.8f), roomTexture);
    
                roomUINode->AddObjectIfMissing<WheelTag>();

                roomUINode->SetEnabled(false);

                mapRooms.push_back({room.position, roomUINode, false});
            }
        }

        PlayerController* player = PlayerController::Instance();
        if (!player) return;

        glm::vec3 playerPosition = player->GlobalTransform().Position().Value();
        float gridSize = dungeonGenerator->gridSize;

        float uiX = (playerPosition.x / gridSize) * mapScale;
        float uiY = (playerPosition.z / gridSize) * mapScale;

        if (auto* indicatorLayout = playerIndicatorNode->GetObject<UiLayout>()) {
            indicatorLayout->offset = glm::ivec2(static_cast<int>(uiX), static_cast<int>(uiY));
        }

        glm::vec2 currentGridPosition(
            std::round(playerPosition.z / gridSize),
            std::round(playerPosition.x / gridSize)
        );

        for (auto& mapRoom : mapRooms) {
            if (!mapRoom.visited && glm::distance(mapRoom.gridPosition, currentGridPosition) < 0.5f) {
                mapRoom.visited = true;
                mapRoom.node->SetEnabled(true);
            }
        }
    }
};
