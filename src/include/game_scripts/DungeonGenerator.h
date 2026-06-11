#pragma once

#include "Debug.h"
#include <queue>

#include <GameObject.h>
#include <Random.h>

class GltfScene;

class ElevatorScript : public GameObject {
private:
	
public:
	void Update();
};

class DungeonGenerator : public GameObject, public ImGuiDrawable {
private:
	enum class RoomShape {
		DeadEnd,
		Corridor,
		Corner,
		TShape,
		Cross
	};

	struct PlacedRoom {
		glm::vec2 position;
		RoomShape type;
		int orientation;
		SceneNode* room;
	};

	struct RoomPrefab {
		GltfScene* prefab;
		RoomShape shape;
		std::string name;
		std::vector<std::string> tags;
		glm::vec2 size;

		inline bool HasTag(const std::string& tag) {
			return std::find(this->tags.begin(), this->tags.end(), tag) != this->tags.end();
		}
	};

	std::vector<RoomPrefab> roomPrefabs;

	std::vector<PlacedRoom> dungeonRooms;
	std::queue<PlacedRoom> roomsToSpawn;

	SceneNode* PlaceRoom();

	RoomPrefab* GetPrefabWithTag(const std::string& tag);
	RoomPrefab* GetPrefabWithShape(RoomShape shape);
public:
	DungeonGenerator() = default;

	std::filesystem::path rootRoomPath = "./res/models/rooms/gardens";

	float gridSize = 40;

	int seed = 1 << 31;
	
	int initialStrideLength = 4;
	int maxFirstShelfOverhang = 1;
	int secondStrideLength = 2;
	int secondStrideLegs = 2;
	int maxSecondShelfOverhang = 2;
	int lastStretchLength = 2;

	void Awake();

	void RemakeDungeon();

	void Update();
	void Render();

	virtual void DrawImGui() override;
};

// #pragma once

// #include "Debug.h"
// #include "GltfImporter.h"
// #include "imgui.h"
// #include "physics/Body.h"
// #include "physics/System.h"
// #include "physics/Helpers.h"

// #include "Jolt/Physics/Body/MotionType.h"
// #include <GameObject.h>
// #include <glm/gtc/random.hpp>
// #include <glm/gtc/quaternion.hpp>
// #include <set>
// #include <vector>
// #include <string>
// #include <ctime>
// #include <format>

// enum class RoomShape {
//     DeadEnd,
//     Corridor,
//     Corner,
//     TShape,
//     Cross
// };

// struct RoomTemplate {
//     std::string modelPath; // Change to a serialized scene later
//     RoomShape shape;
// };

// struct DungeonGeneratorSettings {
//     int mapColumns = 5;
//     int mapRows = 10;
//     int steps = 10;

//     int numberOfBranches = 1;
//     int minBranchLength = 5;
//     int maxBranchLength = 5;

//     float momentum = 2.0f; // How likely is it to keep going the previous direction
//     // How likely is it to go Left/Right or Up/Down
//     float horizontalBias = 0.5f; // 1.0 -> more horizontal, 0.0f -> more vertical

//     std::vector<RoomTemplate> roomPool = {
//         {"./res/models/rooms/room_I.gltf", RoomShape::Corridor},
//         {"./res/models/rooms/room_I_cells.gltf", RoomShape::Corridor},
//         {"./res/models/rooms/room_L.gltf", RoomShape::Corner},
//         {"./res/models/rooms/room_one.gltf", RoomShape::DeadEnd},
//         {"./res/models/rooms/room_T.gltf", RoomShape::TShape},
//         {"./res/models/rooms/room_X.gltf", RoomShape::Cross},
//         {"./res/models/rooms/room_T_Pom.glb", RoomShape::TShape},
//     };
// };

// class DungeonGenerator : public GameObject, public ImGuiDrawable {
// public:
//     DungeonGeneratorSettings settings;
// private:
//     enum class RoomType {
//         Empty,
//         Normal,
//         Start,
//         Goal,
//     };

//     enum class Directions {
//         None,
//         Left,
//         Right,
//         Up,
//         Down,
//     };

//     struct Room {
//     private:
//         struct Doors {
//             bool top = false;
//             bool bottom = false;
//             bool left = false;
//             bool right = false;
//         };
//     public:
//         RoomType roomType = RoomType::Empty;
//         Doors doors;
//     };

//     const float ROOM_SIZE = 28.0f;
//     const int MAX_ATTEMPTS = 100;

//     std::vector<Room> map;
// public:
//     // Move this to awake/enable(?)
//     DungeonGenerator(DungeonGeneratorSettings settings = {}) : settings(settings) {
//         int attempts = 0;
//         while (!this->GenerateDungeon()) {
//             attempts++;
//             if (attempts > this->MAX_ATTEMPTS) {
//                 spdlog::error("DungeonGenerator: Failed to generate a dungeon");
//                 return;
//             }
//         }

//         this->CreateRoomNodes();
//     }

//     virtual ~DungeonGenerator() {}

//     void Regenerate() {
//         for (auto child : this->GetNode()->GetChildren()) {
//             this->GetNode()->GetScene()->DeleteNode(child);
//         }

//         int attempts = 0;
//         while (!this->GenerateDungeon()) {
//             attempts++;
//             if (attempts > this->MAX_ATTEMPTS) {
//                 spdlog::error("DungeonGenerator: Failed to generate a dungeon");
//                 return;
//             }
//         }
//         this->CreateRoomNodes();
//     }

//     void DrawImGui() {
//         ImGui::SliderInt("Steps", &this->settings.steps, 1, 100); 
//         ImGui::SliderInt("Columns", &this->settings.mapColumns, 1, 20);
//         ImGui::SliderInt("Rows", &this->settings.mapRows, 1, 20);
//         ImGui::SliderInt("Number of branches", &this->settings.numberOfBranches, 0, 10);
//         ImGui::SliderInt("Min branch length", &this->settings.minBranchLength, 0, 10);
//         ImGui::SliderInt("Max branch length", &this->settings.maxBranchLength, 0, 10);

//         ImGui::SliderFloat("Momentum", &this->settings.momentum, 0.0f, 10.0f);
//         ImGui::SliderFloat("Horizontal Bias", &this->settings.horizontalBias, 0.0f, 1.0f);

//         if (ImGui::Button("Regenerate Dungeon"))
//             this->Regenerate();
//     }
// private:
//     bool GenerateDungeon() {
//         // Main path
//         std::srand(static_cast<unsigned int>(std::time(nullptr)));

//         const int roomCount = this->settings.mapColumns * this->settings.mapRows;

//         this->map.clear();
//         this->map.resize(roomCount, Room());
//         Room room;

//         // Start position
//         int startPosition = glm::linearRand(0, roomCount - 1);
//         this->map.at(startPosition).roomType = RoomType::Start;
//         std::set<int> visited = { startPosition };

//         this->Traverse(visited, startPosition, this->settings.steps);

//         // Branches
//         for (std::size_t i = 0; i < this->settings.numberOfBranches; ++i) {
//             int attempts = 0;

//             while (true) {
//                 if (attempts >= this->MAX_ATTEMPTS) {
//                     return false;
//                 }

//                 int randomIndex = glm::linearRand(0, static_cast<int>(visited.size() - 1));
//                 startPosition = *std::next(visited.begin(), randomIndex);

//                 if (this->map.at(startPosition).roomType != RoomType::Goal) {
//                     break;
//                 }
//             };

//             int branchLength = glm::linearRand(this->settings.minBranchLength, this->settings.maxBranchLength);
//             Traverse(visited, startPosition, branchLength, false);
//         } 
		
//         return true;
//     }

//     void Traverse(std::set<int>& visited, const int startPosition, const int steps, const bool markGoal = true) {
//         int currentPosition = startPosition;
//         std::vector<int> previousPositions;
//         int step = 0;
//         Directions previousDirection = Directions::None;

//         while (step < steps && step > -1) {
//             bool backtrack = false;

//             std::vector<Directions> availableDirections = this->GetAvailableDirections(currentPosition);

//             while (!availableDirections.empty()) {
//                 float totalWeight = 0.0f;
//                 std::vector<float> weights;
//                 weights.reserve(availableDirections.size());

//                 for (Directions dir : availableDirections) {
//                     float weight = 1.0f;
//                     if (dir == Directions::Left || dir == Directions::Right) {
//                         weight *= this->settings.horizontalBias;
//                     } else {
//                         weight *= (1.0f - this->settings.horizontalBias);
//                     }

//                     if (dir == previousDirection) {
//                         weight *= this->settings.momentum;
//                     }

//                     weights.push_back(weight);
//                     totalWeight += weight;
//                 }

//                 float randomVal = glm::linearRand(0.0f, totalWeight);
//                 float currentWeight = 0.0f;
//                 Directions direction = availableDirections.front();
				
//                 for (std::size_t i = 0; i < availableDirections.size(); ++i) {
//                     currentWeight += weights[i];
//                     if (randomVal <= currentWeight) {
//                         direction = availableDirections[i];
//                         break;
//                     }
//                 }

//                 int nextPosition = currentPosition;
//                 bool moved = false;
//                 switch (direction) {
//                     case Directions::Up:
//                         nextPosition = currentPosition - this->settings.mapColumns;
//                         if (!visited.contains(nextPosition) && nextPosition >= 0 && nextPosition < map.size()) {
//                             map.at(currentPosition).doors.top = true;
//                             map.at(nextPosition).doors.bottom = true;
//                             currentPosition = nextPosition;
//                             previousPositions.push_back(currentPosition);
//                             visited.insert(currentPosition);
//                             // temporary
//                             map.at(currentPosition).roomType = RoomType::Normal;
//                             previousDirection = Directions::Up;
//                             availableDirections.clear();
//                             moved = true;
//                         } else {
//                             std::erase(availableDirections, Directions::Up);
//                         }
//                         break;
//                     case Directions::Down:
//                         nextPosition = currentPosition + this->settings.mapColumns;
//                         if (!visited.contains(nextPosition) && nextPosition >= 0 && nextPosition < map.size()) {
//                             map.at(currentPosition).doors.bottom = true;
//                             map.at(nextPosition).doors.top = true;
//                             currentPosition = nextPosition;
//                             previousPositions.push_back(currentPosition);
//                             visited.insert(currentPosition);
//                             map.at(currentPosition).roomType = RoomType::Normal;
//                             previousDirection = Directions::Down;
//                             availableDirections.clear();
//                             moved = true;
//                         } else {
//                             std::erase(availableDirections, Directions::Down);
//                         }
//                         break;
//                     case Directions::Left:
//                         nextPosition = currentPosition - 1;
//                         if (!visited.contains(nextPosition) && nextPosition >= 0 && nextPosition < map.size()) {
//                             map.at(currentPosition).doors.left = true;
//                             map.at(nextPosition).doors.right = true;
//                             currentPosition = nextPosition;
//                             previousPositions.push_back(currentPosition);
//                             visited.insert(currentPosition);
//                             map.at(currentPosition).roomType = RoomType::Normal;
//                             previousDirection = Directions::Left;
//                             availableDirections.clear();
//                             moved = true;
//                         } else {
//                             std::erase(availableDirections, Directions::Left);
//                         }
//                         break;
//                     case Directions::Right:
//                         nextPosition = currentPosition + 1;
//                         if (!visited.contains(nextPosition) && nextPosition >= 0 && nextPosition < map.size()) {
//                             map.at(currentPosition).doors.right = true;
//                             map.at(nextPosition).doors.left = true;
//                             currentPosition = nextPosition;
//                             previousPositions.push_back(currentPosition);
//                             visited.insert(currentPosition);
//                             // temporary
//                             map.at(currentPosition).roomType = RoomType::Normal;
//                             previousDirection = Directions::Right;
//                             availableDirections.clear();
//                             moved = true;
//                         } else {
//                             std::erase(availableDirections, Directions::Right);
//                         }
//                         break;
//                     default:
//                         break;
//                 }

//                 if (availableDirections.empty() && !previousPositions.empty() && moved == false) {
//                     currentPosition = previousPositions.back();
//                     previousPositions.pop_back();
//                     previousDirection = Directions::None;
//                     backtrack = true;
//                 } else if (availableDirections.empty() && previousPositions.empty() && moved == false) {
//                     break;
//                 }
//             }

//             if (backtrack == true)
//                 continue;
			
//             if (step == steps - 1 && markGoal == true) {
//                 map.at(currentPosition).roomType = RoomType::Goal;
//             }
			
//             step++;
//         }
//     }

//     std::vector<Directions> GetAvailableDirections(const int currentPosition) {
//         std::vector<Directions> availableDirections;

//         if (currentPosition >= this->settings.mapColumns)
//             availableDirections.push_back(Directions::Up);

//         const int roomCount = this->settings.mapColumns * this->settings.mapRows;
//         if (currentPosition < roomCount - this->settings.mapColumns)
//             availableDirections.push_back(Directions::Down);

//         if (currentPosition % this->settings.mapColumns != 0)
//             availableDirections.push_back(Directions::Left);

//         if ((currentPosition + 1) % settings.mapColumns != 0)
//             availableDirections.push_back(Directions::Right);

//         return availableDirections;
//     }

//     void CreateRoomNodes() {
//         const float offsetX = (this->settings.mapColumns * (this->ROOM_SIZE) * 0.5f) + this->ROOM_SIZE * 0.5f;
//         const float offsetZ = (this->settings.mapRows * (this->ROOM_SIZE) * 0.5f) + this->ROOM_SIZE * 0.5f;
//         glm::vec3 startPosition = {
//             -offsetX,
//             0.0f,
//             -offsetZ,
//         };

//         for (std::size_t y = 0; y < this->settings.mapRows; y++) {
//             for (std::size_t x = 0; x < this->settings.mapColumns; x++) {
//                 const std::size_t index = y * this->settings.mapColumns + x;
//                 Room& room = this->map.at(index);

//                 if (room.roomType == RoomType::Empty)
//                     continue;

//                 int doorCount = room.doors.top + room.doors.bottom + room.doors.left + room.doors.right;
//                 if (doorCount == 0) continue;

//                 RoomShape requiredShape = RoomShape::DeadEnd;
//                 float rotationDegrees = 0.0f;

//                 if (doorCount == 1) {
//                     requiredShape = RoomShape::DeadEnd;
//                     if (room.doors.top) rotationDegrees = 90.0f;
//                     else if (room.doors.right) rotationDegrees = 0.0f;
//                     else if (room.doors.bottom) rotationDegrees = -90.0f;
//                     else if (room.doors.left) rotationDegrees = 180.0f;
//                 } else if (doorCount == 2) {
//                     if (room.doors.top && room.doors.bottom) {
//                         requiredShape = RoomShape::Corridor;
//                         rotationDegrees = 90.0f;
//                     } else if (room.doors.left && room.doors.right) {
//                         requiredShape = RoomShape::Corridor;
//                         rotationDegrees = 0.0f;
//                     } else {
//                         requiredShape = RoomShape::Corner;
//                         if (room.doors.top && room.doors.right) rotationDegrees = 0.0f;
//                         else if (room.doors.right && room.doors.bottom) rotationDegrees = -90.0f;
//                         else if (room.doors.bottom && room.doors.left) rotationDegrees = 180.0f;
//                         else if (room.doors.left && room.doors.top) rotationDegrees = 90.0f;
//                     }
//                 } else if (doorCount == 3) {
//                     requiredShape = RoomShape::TShape;
//                     if (!room.doors.bottom) rotationDegrees = 0.0f;
//                     else if (!room.doors.left) rotationDegrees = -90.0f;
//                     else if (!room.doors.top) rotationDegrees = 180.0f;
//                     else if (!room.doors.right) rotationDegrees = 90.0f;
//                 } else if (doorCount == 4) {
//                     requiredShape = RoomShape::Cross;
//                     rotationDegrees = 0.0f;
//                 }

//                 std::vector<std::string> matchingPaths;
//                 for (const auto& roomTemplate : this->settings.roomPool) {
//                     if (roomTemplate.shape == requiredShape) {
//                         matchingPaths.push_back(roomTemplate.modelPath);
//                     }
//                 }

//                 std::string modelPath = "";
//                 if (!matchingPaths.empty()) {
//                     int randomIndex = glm::linearRand(0, static_cast<int>(matchingPaths.size() - 1));
//                     modelPath = matchingPaths[randomIndex];
//                 }

//                 if (modelPath.empty()) {
//                     spdlog::warn("DungeonGenerator: No matching room template found for shape at {}x{}", x, y);
//                     continue;
//                 }

//                 SceneNode* scene = GltfImporter::LoadScene(
//                     this->GetScene(),
//                     modelPath,
//                     std::format("Room {}x{}", x, y),
//                     this->GetNode()
//                 );

//                 const float roomSize = this->ROOM_SIZE;
//                 const glm::vec3 positionOffset = {
//                     x * roomSize, 
//                     0.0f,
//                     y * roomSize, 
//                 };
				
//                 scene->LocalTransform().Position() = startPosition + positionOffset;
				
//                 if (rotationDegrees != 0.0f) {
//                     scene->LocalTransform().Rotation() = glm::quat(glm::vec3(0.0f, glm::radians(rotationDegrees), 0.0f));
//                 }

//                 JPH::ShapeRefC shape = Physics::CreateCompoundShapeFromNode(scene, false, JPH::EMotionType::Static, Physics::Layers::NON_MOVING);
//                 JPH::BodyCreationSettings settings = {
//                     shape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Physics::Layers::NON_MOVING
//                 };
//                 scene->AddObject<Physics::Body>(settings);
//             }
//         }
//     }
// };
