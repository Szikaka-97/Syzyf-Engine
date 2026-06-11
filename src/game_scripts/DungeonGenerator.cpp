#include "Surface.h"
#include "game_scripts/PlayerController.h"
#include "game_scripts/enemies/FlockingSystem.h"
#include <game_scripts/DungeonGenerator.h>

#include <numeric>

#include <GltfScene.h>
#include <Formatters.h>
#include <imgui.h>

#include <physics/Helpers.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <physics/Body.h>
#include <game_scripts/enemies/EnemySkeleton.h>

SceneNode* DungeonGenerator::PlaceRoom() {
	return nullptr;
}

DungeonGenerator::RoomPrefab* DungeonGenerator::GetPrefabWithTag(const std::string& tag) {
	auto roomIt = std::find_if(this->roomPrefabs.begin(), this->roomPrefabs.end(), [&tag](RoomPrefab prefab) -> bool { return prefab.HasTag(tag); } );

	if (roomIt != this->roomPrefabs.end()) {
		return &*roomIt;
	}

	return nullptr;
}
DungeonGenerator::RoomPrefab* DungeonGenerator::GetPrefabWithShape(RoomShape shape) {
	auto roomIt = std::find_if(this->roomPrefabs.begin(), this->roomPrefabs.end(), [&shape](RoomPrefab prefab) -> bool { return prefab.shape == shape; } );

	if (roomIt != this->roomPrefabs.end()) {
		return &*roomIt;
	}

	return nullptr;
}

struct glmvec2compare {
	constexpr bool operator()(const glm::vec2& a, const glm::vec2& b) const {
		if (a.y == b.y) {
			return a.x < b.x;
		}
	
		return a.y < b.y;
	}
};

void DungeonGenerator::RemakeDungeon() {
	constexpr glm::vec2 vec_forward = glm::vec2(0, 1);
	constexpr glm::vec2 vec_back = glm::vec2(0, -1);
	constexpr glm::vec2 vec_right = glm::vec2(1, 0);
	constexpr glm::vec2 vec_left = glm::vec2(-1, 0);

	Random rng;

	for (auto room : this->dungeonRooms) {
		delete room.room;
	}

	this->dungeonRooms.clear();

	std::map<glm::vec2, PlacedRoom, glmvec2compare> placedRooms;

	glm::vec2 currentPosition = glm::vec2(0, 0);

	placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::DeadEnd, 0 };

	int firstStride = rng.ValueInt(1, this->initialStrideLength);

	for (int i = 0; i < firstStride; i++) {
		currentPosition += vec_forward;

		placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::Corridor, 0 };
	}

	float firstShelfOverhangLeft = rng.ValueInt(1, this->maxFirstShelfOverhang);
	float firstShelfOverhangRight = rng.ValueInt(1, this->maxFirstShelfOverhang);

	currentPosition += vec_forward + vec_left * firstShelfOverhangLeft;

	placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::DeadEnd, 1 };

	currentPosition += vec_right;

	for (int i = 1; i < firstShelfOverhangLeft + firstShelfOverhangRight; i++) {
		if (i == firstShelfOverhangLeft) {
			placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::TShape, 2 };
		}
		else {
			placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::Corridor, 1 };
		}

		currentPosition += vec_right;
	}

	placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::DeadEnd, 3 };

	currentPosition -= vec_right * (firstShelfOverhangLeft + firstShelfOverhangRight + 1);
	
	int lastReturnX = -firstShelfOverhangLeft - 2;
	for (int distFromStart = -firstShelfOverhangLeft; distFromStart <= firstShelfOverhangRight; distFromStart++) {
		currentPosition += vec_right;

		if (distFromStart <= lastReturnX + 1 || std::abs(distFromStart) < 2) {
			continue;
		}

		if (rng.Chance(0.5)) {
			placedRooms[currentPosition].type = placedRooms[currentPosition].type == RoomShape::Corridor ? RoomShape::TShape : RoomShape::Corner;
			placedRooms[currentPosition].orientation = distFromStart != -firstShelfOverhangLeft ? 2 : 1;

			int returnJourney = rng.ValueInt(1, firstStride);

			lastReturnX = distFromStart;

			for (int i = 0; i < returnJourney; i++) {
				currentPosition += vec_back;

				placedRooms[currentPosition] = PlacedRoom{ currentPosition, i < returnJourney - 1 ? RoomShape::Corridor : RoomShape::DeadEnd, 0 };
			}

			currentPosition += vec_forward * (float) returnJourney;
		}
	}

	currentPosition -= vec_right * (firstShelfOverhangLeft + firstShelfOverhangRight);

	int secondStride = rng.ValueInt(2, std::max(2, this->secondStrideLength));

	int secondStrideLegCount = rng.ValueInt(1, this->secondStrideLegs);

	glm::ivec2 longestStride = glm::ivec2(0, 0);

	std::vector<int> legPositions(firstShelfOverhangLeft + firstShelfOverhangRight);
	std::iota(legPositions.begin(), legPositions.end(), 1);

	rng.Shuffle(legPositions);

	for (int i = 0; i < secondStrideLegCount; i++) {
		glm::vec2 legPosition = currentPosition + vec_right * float(legPositions[i]);

		int strideLength = rng.ValueInt(2, secondStride + 1);

		if (strideLength > longestStride.y) {
			longestStride.y = strideLength;
			longestStride.x = legPositions[i];
		}

		if (placedRooms[legPosition].type == RoomShape::DeadEnd) {
			placedRooms[legPosition].type = RoomShape::Corner;
			placedRooms[legPosition].orientation = currentPosition.x < 0 ? 0 : 3;
		}
		else if (placedRooms[legPosition].type == RoomShape::Corner) {
			placedRooms[legPosition].type = RoomShape::TShape;
			placedRooms[legPosition].orientation = currentPosition.x < 0 ? 1 : 3;
		}
		else if (placedRooms[legPosition].type == RoomShape::Corridor) {
			placedRooms[legPosition].type = RoomShape::TShape;
			placedRooms[legPosition].orientation = 0;
		}
		else if (placedRooms[legPosition].type == RoomShape::TShape) {
			placedRooms[legPosition].type = RoomShape::Cross;
			placedRooms[legPosition].orientation = 0;
		}

		for (int j = 0; j < strideLength; j++) {
			legPosition += vec_forward;

			placedRooms[legPosition] = PlacedRoom{ legPosition, RoomShape::Corridor, 0 };
		}
	}

	int secondShelfOverhangLeft = firstShelfOverhangLeft;
	int secondShelfOverhangRight = firstShelfOverhangRight;
	
	if (this->secondStrideLength > 0) {
		int secondShelfOverhangLeft = rng.ValueInt(1, this->maxSecondShelfOverhang);
		int secondShelfOverhangRight = rng.ValueInt(1, this->maxSecondShelfOverhang);
	
		currentPosition += vec_right * (float) longestStride.x + vec_forward * (float) (longestStride.y + 1);
	
		currentPosition -= vec_right * (float) secondShelfOverhangLeft;
	
		for (int i = 0; i < secondShelfOverhangLeft + secondShelfOverhangRight + 1; i++) {
			if (placedRooms.contains(currentPosition - vec_forward)) {
				placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::TShape, 2 };
			}
			else {
				placedRooms[currentPosition] = PlacedRoom{ currentPosition, i == 0 ? RoomShape::DeadEnd : RoomShape::Corridor, 1 };
			}
	
			currentPosition += vec_right;
		}
	
		placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::DeadEnd, 3 };
	}
	
	int lastStretchX = rng.ValueInt(1, secondShelfOverhangLeft + secondShelfOverhangRight + 1);

	int lastStretchY = rng.ValueInt(1, this->lastStretchLength + 1);

	currentPosition += vec_left * (float) lastStretchX;
	
	if (placedRooms[currentPosition].type == RoomShape::Corridor) {
		placedRooms[currentPosition].type = RoomShape::TShape;
		placedRooms[currentPosition].orientation = 0;
	}
	else if (placedRooms[currentPosition].type == RoomShape::TShape) {
		placedRooms[currentPosition].type = RoomShape::Cross;
	}
	else if (placedRooms[currentPosition].type == RoomShape::DeadEnd) {
		placedRooms[currentPosition].type = RoomShape::Corner;
	}

	for (int i = 0; i < lastStretchY; i++) {
		currentPosition += vec_forward;

		placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::Corridor, 0 };
	}

	currentPosition += vec_forward;

	placedRooms[currentPosition] = PlacedRoom{ currentPosition, RoomShape::DeadEnd, 2 };

	for (auto room : placedRooms) {
		roomsToSpawn.push(room.second);
	}
}

void SpawnEnemy(SceneNode* position) {
	JPH::ShapeRefC enemyShape = new JPH::CapsuleShape(0.5f, 1.0f);
    JPH::BodyCreationSettings enemySettings(
        enemyShape, JPH::RVec3(10.5f, 2.0f, 2.0f), JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
    Material* enemyMat =
        position->GetScene()->Resources()->Get<Material>("./res/materials/jake.mat");
    Mesh* cubeMesh =
        position->GetScene()->Resources()->Get<Mesh>("./res/models/not_cube.obj");

    SceneNode* enemy1 = position->GetScene()->CreateNode(position, "Enemy 1");
    // enemy1->GlobalTransform().Position() = glm::vec3(10.5f, 0.0f, -5.0f);
    enemy1->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
    enemy1->GlobalTransform().Position() = position->GlobalTransform().Position().Value() + glm::vec3(0, 1, 0);
    Physics::Body* enemyBody1 = enemy1->AddObject<Physics::Body>(enemySettings);
    enemyBody1->SetRestitution(0.0f);
    auto* enemyAi1 = enemy1->AddObject<EnemySkeleton>();
    enemyAi1->SetProjectileResources(cubeMesh, enemyMat);
    enemyAi1->SetSurface(position->GetParent()->GetObjectInChildren<Surface>());
    enemyAi1->SetProjectileResources(cubeMesh, enemyMat);
	enemyAi1->SetTargetNode(PlayerController::Instance()->GetNode());
    enemyAi1->SetAttackCooldown(1.2f);
	enemyAi1->m_FlockingSystem = position->GetScene()->GetComponent<FlockingSystem>();
	enemyAi1->SetRoomID(enemyAi1->GetSurface()->GetID());

    enemyAi1->RegisterToFlockingSystem(position->GetScene()->GetComponent<FlockingSystem>());

    SceneNode* enemyModel =
        ResourceDatabase::Global->Get<GltfScene>("./res/models/szkielet6.glb")
            ->Instantiate(position->GetScene(), enemy1, "EnemyModel");
    enemyModel->SetParent(enemy1);
    enemyModel->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);
    enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();
}

void DungeonGenerator::Awake() {
	for (const auto& roomFile : std::filesystem::directory_iterator(this->rootRoomPath)) {
		if (roomFile.path().extension() == ".glb") {
			RoomPrefab prefab;

			prefab.prefab = GetScene()->Resources()->Get<GltfScene>(roomFile);

			SceneNode* instantiatedRoom = prefab.prefab->Instantiate(GetScene(), nullptr, roomFile.path().stem().string());

			for (SceneNode* child : instantiatedRoom->GetChildren()) {
				if (child->GetName() == "ROOM_SHAPE_O") {
					prefab.shape = RoomShape::DeadEnd;
				}
				else if (child->GetName() == "ROOM_SHAPE_I") {
					prefab.shape = RoomShape::Corridor;
				}
				else if (child->GetName() == "ROOM_SHAPE_L") {
					prefab.shape = RoomShape::Corner;
				}
				else if (child->GetName() == "ROOM_SHAPE_T") {
					prefab.shape = RoomShape::TShape;
				}
				else if (child->GetName() == "ROOM_SHAPE_X") {
					prefab.shape = RoomShape::Cross;
				}
				else if (child->GetName().starts_with("ROOM_SIZE_")) {
					std::stringstream sizeString(child->GetName().substr(10));

					std::string sizeX;
					std::string sizeY;

					std::getline(sizeString, sizeX, 'x');
					std::getline(sizeString, sizeY);

					prefab.size.x = std::stof(sizeX);
					prefab.size.y = std::stof(sizeY);
				}
				else if (child->GetName().starts_with("ROOM_TAG_")) {
					prefab.tags.push_back(child->GetName().substr(9));
				}
			}

			this->roomPrefabs.push_back(prefab);

			delete instantiatedRoom;
		}
	}
}

void DungeonGenerator::Update() {
	glm::vec3 playerPosition = PlayerController::Instance()->GlobalTransform().Position();

	for (auto room : this->dungeonRooms) {
		glm::vec3 roomCenter = glm::vec3(room.position.y, 0, room.position.x) * glm::vec3(this->gridSize);

		glm::vec3 dist = glm::abs(roomCenter - playerPosition);

		dist.y = 0;

		if (dist.x > this->gridSize * 1.5 || dist.z > this->gridSize * 1.5) {
			room.room->SetEnabled(false);
		}
	}
}

void DungeonGenerator::Render() {
	static int roomCounter = 0;

	if (!this->roomsToSpawn.empty()) {
		auto room = this->roomsToSpawn.front();
		this->roomsToSpawn.pop();

		GltfScene* roomPrefab = nullptr;

		if (room.position == glm::vec2(0, 0)) {
			roomPrefab = GetPrefabWithTag("START")->prefab;
		}
		else {
			roomPrefab = GetPrefabWithShape(room.type)->prefab;
		}

		roomCounter++;

		if (roomPrefab) {
			spdlog::info("Room coords: {}", room.position);

			SceneNode* spawnedRoom = roomPrefab->Instantiate(GetScene(), GetNode(), std::format("Room {}", roomCounter));
			
			spawnedRoom->GlobalTransform().Position() = GlobalTransform().Position() + glm::vec3(room.position.y, 0, room.position.x) * glm::vec3(this->gridSize);
			spawnedRoom->GlobalTransform().Rotation() = glm::vec3(0, glm::radians(-90.0f * room.orientation), 0);

			SceneNode* floorNode = nullptr;

			if (spawnedRoom->TryFindNode("FLOOR", &floorNode)) {
				floorNode->AddObject<Surface>(floorNode->GetObject<MeshRenderer>()->GetMesh(), 1);
			}

			for (MeshRenderer* mesh : spawnedRoom->GetAllObjectsInChildren<MeshRenderer>()) {
				auto* body = mesh->GetNode()->AddObject<Physics::Body>(
				JPH::BodyCreationSettings{
					Physics::MeshShape(mesh->GetMesh()),
					JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
					Physics::Layers::NON_MOVING});
				
				body->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
			}

			for (SceneNode* child : spawnedRoom->GetChildren()) {
				if (child->GetName().starts_with("ENEMY_SPAWN_Skeleton")) {
					SpawnEnemy(child);
				}
			}

			room.room = spawnedRoom;

			this->dungeonRooms.push_back(room);
		}
	}
}

void DungeonGenerator::DrawImGui() {
	ImGui::InputFloat("Grid Size", &this->gridSize, 1);

	ImGui::InputInt("Max Initial Stride Length", &this->initialStrideLength);
	ImGui::InputInt("Max First Shelf Overhang", &this->maxFirstShelfOverhang);
	ImGui::InputInt("Max Secondary Stride Length", &this->secondStrideLength);
	ImGui::InputInt("Max Second Stride Legs Count", &this->secondStrideLegs);
	ImGui::InputInt("Max Second Shelf Overhang", &this->maxSecondShelfOverhang);
	ImGui::InputInt("Max Last Stretch Length", &this->lastStretchLength);

	bool enableSeed = !(this->seed & (1 << 31));

	ImGui::Checkbox("Enable Seed", &enableSeed);
	
	if (enableSeed) {
		ImGui::InputInt("Seed", &this->seed);
	}
	else {
		this->seed |= 1 << 31;
	}
	
	if (ImGui::Button("Remake Dungeon")) {
		RemakeDungeon();
	}
}