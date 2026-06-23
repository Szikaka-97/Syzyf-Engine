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
#include <game_scripts/enemies/EnemyBeetroot.h>
#include <game_scripts/enemies/EnemyPotato.h>

#include <MathHelpers.h>
#include <TimeSystem.h>
#include <Application.h>


namespace CraftingScene {
inline void InitScene(Scene& mainScene);
}


DungeonRoomScript::DungeonRoomScript() {
	this->surface = GetNode()->FindNode("floor")->GetObject<Surface>();

	for (auto* node : GetNode()->GetChildren()) {
		if (node->GetName().starts_with("gate")) {
			this->doors.push_back(node);
		}
	}
}

void DungeonRoomScript::Update() {
	if (this->surface->PlayerInside()) {
		if (!this->enemiesSpawned) {
			SpawnEnemies();
		}

		this->timeout -= Time::Delta();

		if (this->timeout > 0) {
			return;
		}

		if (this->surface->GetEnemies().size() > 0) {
			for (auto* door : this->doors) {
				glm::vec3 pos = door->GetObject<Physics::Body>()->GetPosition();
				
				pos.y = Math::MoveTowards(pos.y, 1.4f, Time::Delta() * 10);

				door->GetObject<Physics::Body>()->SetPosition(pos);
				door->GlobalTransform().Position() = pos;
			}
		}
		else {
			if (this->isFinal) {
				Application::Get()->RequestSceneBuild(
					[](Scene* s) { CraftingScene::InitScene(*s); }
				);
			}

			for (auto* door : this->doors) {
				glm::vec3 pos = door->GetObject<Physics::Body>()->GetPosition();
				
				pos.y = Math::MoveTowards(pos.y, 3.8f, Time::Delta() * 10);

				door->GetObject<Physics::Body>()->SetPosition(pos);
				door->GlobalTransform().Position() = pos;
			}
		}
	}
}

void ElevatorScript::Update() {
	glm::vec3 pos = GlobalTransform().Position();

	if (pos.y > 0) {
		pos.y = Math::MoveTowards(pos.y, 0, Time::Delta());

		PlayerController::Instance()->GlobalTransform().Position() = GlobalTransform().Position().Value();
		
		GlobalTransform().Position() = pos;
	}
	else {
		PlayerController::Instance()->SetEnabled(true);

		delete this;
	}

}

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

enum class EnemySpawnType {
    Skeleton,
    Rat,
    Beet,
    Potato
};

static EnemySpawnType GetEnemySpawnType(SceneNode* spawnNode) {
    const std::string& name = spawnNode->GetName();

    if (name.starts_with("ENEMY_SPAWN_Sceleton")) {
        return EnemySpawnType::Skeleton;
    }

    // Obsługa poprawnej pisowni, gdybyś później zmienił nazwę w Blenderze.
    if (name.starts_with("ENEMY_SPAWN_Skeleton")) {
        return EnemySpawnType::Skeleton;
    }

    if (name.starts_with("ENEMY_SPAWN_Rat")) {
        return EnemySpawnType::Rat;
    }

    if (name.starts_with("ENEMY_SPAWN_Beet")) {
        return EnemySpawnType::Beet;
    }

    if (name.starts_with("ENEMY_SPAWN_Potato")) {
        return EnemySpawnType::Potato;
    }

    // Domyślnie szkielet, żeby stare ENEMY_SPAWN_ też dalej działały.
    return EnemySpawnType::Skeleton;
}

static const char* GetEnemyModelPath(EnemySpawnType type) {
    switch (type) {
        case EnemySpawnType::Skeleton:
            return "./res/models/enemies/szkielet4.glb";

        case EnemySpawnType::Rat:
            return "./res/models/enemies/rat6.glb";

        case EnemySpawnType::Beet:
            return "./res/models/enemies/burak_macki3_bisect.glb";

        case EnemySpawnType::Potato:
            return "./res/models/enemies/ziemniak_remake4.glb";

        default:
            return "./res/models/enemies/szkielet4.glb";
    }
}

static const char* GetEnemyNodeName(EnemySpawnType type) {
    switch (type) {
        case EnemySpawnType::Skeleton:
            return "Enemy Skeleton";

        case EnemySpawnType::Rat:
            return "Enemy Rat";

        case EnemySpawnType::Beet:
            return "Enemy Beetroot";

        case EnemySpawnType::Potato:
            return "Enemy Potato";

        default:
            return "Enemy";
    }
}

static glm::vec3 GetEnemyModelScale(EnemySpawnType type) {
    switch (type) {
        case EnemySpawnType::Skeleton:
            return glm::vec3(1.0f);

        case EnemySpawnType::Rat:
            return glm::vec3(1.0f);

        case EnemySpawnType::Beet:
            return glm::vec3(1.0f);

        case EnemySpawnType::Potato:
            return glm::vec3(1.0f);

        default:
            return glm::vec3(1.0f);
    }
}

EnemyBase* SpawnEnemy(SceneNode* position) {
    Scene* scene = position->GetScene();

    EnemySpawnType spawnType = GetEnemySpawnType(position);

    Material* enemyMat =
        scene->Resources()->Get<Material>("./res/materials/jake.mat");

    Mesh* cubeMesh =
        scene->Resources()->Get<Mesh>("./res/models/not_cube.obj");

    SceneNode* enemy1 =
        scene->CreateNode(position, GetEnemyNodeName(spawnType));

    glm::vec3 spawnPos =
        position->GlobalTransform().Position().Value() + glm::vec3(0.0f, 1.0f, 0.0f);

    enemy1->GlobalTransform().Scale() = glm::vec3(1.0f);
    enemy1->GlobalTransform().Position() = spawnPos;

    JPH::ShapeRefC enemyShape = new JPH::CapsuleShape(0.25f, 0.5f);

    JPH::BodyCreationSettings enemySettings(
        enemyShape,
        JPH::RVec3(spawnPos.x, spawnPos.y, spawnPos.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Physics::Layers::MOVING
    );

    enemySettings.mAllowedDOFs =
        JPH::EAllowedDOFs::TranslationX |
        JPH::EAllowedDOFs::TranslationY |
        JPH::EAllowedDOFs::TranslationZ |
        JPH::EAllowedDOFs::RotationY;

    enemySettings.mAngularDamping = 10.0f;
    enemySettings.mLinearDamping = 0.5f;

    Physics::Body* enemyBody = enemy1->AddObject<Physics::Body>(enemySettings);
    enemyBody->SetRestitution(0.0f);
    enemyBody->SetFriction(0.8f);

    EnemyBase* enemyAi = nullptr;

    switch (spawnType) {
        case EnemySpawnType::Skeleton: {
            auto* skeleton = enemy1->AddObject<EnemySkeleton>();

            skeleton->m_hp = 5;
            skeleton->SetAttackCooldown(1.2f);

            enemyAi = skeleton;
            break;
        }

        case EnemySpawnType::Rat: {
            // Rat używa tej samej logiki co szkielet,
            // ale ładuje model szczura.
            auto* rat = enemy1->AddObject<EnemySkeleton>();

            rat->m_hp = 5;
            rat->SetAttackCooldown(1.2f);

            enemyAi = rat;
            break;
        }

        case EnemySpawnType::Beet: {
            auto* beet = enemy1->AddObject<EnemyBeetroot>();

            beet->m_hp = 100;

            // Podmień ścieżki, jeśli Twoje pliki nazywają się inaczej.
            Mesh* segmentMesh =
                scene->Resources()->Get<Mesh>(
                    "./res/models/enemies/beetroot_segment.obj"
                );

            Material* segmentMat =
                scene->Resources()->Get<Material>(
                    "./res/materials/beetroot_segment.mat"
                );

            beet->SetSegmentResources(segmentMesh, segmentMat);

            enemyAi = beet;
            break;
        }

        case EnemySpawnType::Potato: {
            auto* potato = enemy1->AddObject<EnemyPotato>();

            potato->m_hp = 100;

            // Podmień ścieżki, jeśli Twoje pliki nazywają się inaczej.
            Mesh* shadowMesh =
                scene->Resources()->Get<Mesh>(
                    "./res/models/enemies/potato_shadow.obj"
                );

            Material* shadowMat =
                scene->Resources()->Get<Material>(
                    "./res/materials/potato_shadow.mat"
                );

            potato->SetShadowResources(shadowMesh, shadowMat);

            enemyAi = potato;
            break;
        }
    }

    if (!enemyAi) {
        spdlog::warn(
            "SpawnEnemy: failed to create enemy from marker '{}'",
            position->GetName()
        );

        return nullptr;
    }

    enemyAi->SetProjectileResources(cubeMesh, enemyMat);
    enemyAi->SetTargetNode(PlayerController::Instance()->GetNode());

    FlockingSystem* flocking = scene->GetComponent<FlockingSystem>();

    enemyAi->m_FlockingSystem = flocking;

    if (flocking) {
        enemyAi->RegisterToFlockingSystem(flocking);
    }
    else {
        spdlog::warn("SpawnEnemy: FlockingSystem not found");
    }

    const char* modelPath = GetEnemyModelPath(spawnType);

    SceneNode* enemyModel =
        ResourceDatabase::Global->Get<GltfScene>(modelPath)
            ->Instantiate(scene, enemy1, "EnemyModel");

    enemyModel->SetParent(enemy1);
    enemyModel->GlobalTransform().Scale() = GetEnemyModelScale(spawnType);
    enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();

	auto animationComponents =
	enemy1->GetAllObjectsInChildren<AnimationComponent>();

	AnimationComponent* anim = nullptr;

	for (AnimationComponent* candidate : animationComponents) {
		if (!candidate->animations.empty()) {
			anim = candidate;
			break;
		}
	}

	if (anim) {
		enemyAi->SetAttackAnimation(anim);

		spdlog::info(
		    "SpawnEnemy: AnimationComponent connected to enemy '{}'",
		    enemy1->GetName()
		);

		for (const auto& a : anim->animations) {
			spdlog::info(
			    "SpawnEnemy: enemy '{}' has animation '{}', duration {:.2f}",
			    enemy1->GetName(),
			    a.data.name,
			    a.data.duration
			);
		}
	}
	else {
		spdlog::warn(
		    "SpawnEnemy: no usable AnimationComponent found in enemy model '{}'",
		    enemy1->GetName()
		);
	}

    spdlog::info(
        "SpawnEnemy: spawned '{}' from marker '{}'",
        enemy1->GetName(),
        position->GetName()
    );

    return enemyAi;
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

			instantiatedRoom->SetEnabled(false);
		}
	}
}

void DungeonGenerator::Update() {
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

			SceneNode* spawnedRoom =
				roomPrefab->Instantiate(
					GetScene(),
					GetNode(),
					std::format("Room {}", roomCounter)
				);

			spawnedRoom->GlobalTransform().Position() =
				GlobalTransform().Position() +
				glm::vec3(room.position.y, 0, room.position.x) *
				glm::vec3(this->gridSize);

			spawnedRoom->GlobalTransform().Rotation() =
				glm::vec3(0, glm::radians(-90.0f * room.orientation), 0);

			SceneNode* floorNode = nullptr;

			if (spawnedRoom->TryFindNode("floor", &floorNode)) {
				floorNode->AddObject<Surface>(
					floorNode->GetObject<MeshRenderer>()->GetMesh(),
					1
				);
			}

			if (room.position == glm::vec2(0, 0)) {
				auto* elevatorNode = spawnedRoom->FindNode("Elevator");

				// elevatorNode->AddObject<ElevatorScript>();
				elevatorNode->GlobalTransform().Position() = glm::vec3(0, 0, 0);

				// PlayerController::Instance()->SetEnabled(false);
				// PlayerController::Instance()->GlobalTransform().Position() =
				//     elevatorNode->GlobalTransform().Position().Value();
			}
			else {
				auto* roomScript = spawnedRoom->AddObject<DungeonRoomScript>();

				if (this->roomsToSpawn.empty()) {
					roomScript->isFinal = true;
				}
			}

			for (MeshRenderer* mesh : spawnedRoom->GetAllObjectsInChildren<MeshRenderer>()) {
				auto* body = mesh->GetNode()->AddObject<Physics::Body>(
					JPH::BodyCreationSettings{
						Physics::MeshShape(mesh->GetMesh()),
						JPH::RVec3::sZero(),
						JPH::Quat::sZero(),
						JPH::EMotionType::Static,
						Physics::Layers::NON_MOVING
					}
				);

				body->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
			}

			// Enemy NIE są już spawnowane tutaj.
			// Spawn odpali się dopiero w DungeonRoomScript::SpawnEnemies(),
			// gdy gracz wejdzie do pokoju.

			room.room = spawnedRoom;

			this->dungeonRooms.push_back(room);
		}
	}

	glm::vec3 playerPosition =
		PlayerController::Instance()->GlobalTransform().Position();

	for (auto room : this->dungeonRooms) {
		glm::vec3 roomCenter =
			glm::vec3(room.position.y, 0, room.position.x) *
			glm::vec3(this->gridSize);

		glm::vec3 dist = glm::abs(roomCenter - playerPosition);
		dist.y = 0;

		if (dist.x < this->gridSize * 1.5 &&
			dist.z < this->gridSize * 1.5) {
			room.room->SetEnabled(true);
		}
		else {
			room.room->SetEnabled(false);
		}
	}
}

void DungeonGenerator::Render() {
	
}

void DungeonRoomScript::SpawnEnemies() {
	if (this->enemiesSpawned) {
		return;
	}

	this->enemiesSpawned = true;

	if (!this->surface) {
		spdlog::warn(
		    "DungeonRoomScript: surface is null in room '{}'",
		    GetNode()->GetName()
		);
		return;
	}

	for (SceneNode* child : GetNode()->GetChildren()) {
		if (!child->GetName().starts_with("ENEMY_SPAWN_")) {
			continue;
		}

		EnemyBase* enemy = SpawnEnemy(child);

		if (!enemy) {
			spdlog::warn(
			    "DungeonRoomScript: failed to spawn enemy from '{}'",
			    child->GetName()
			);
			continue;
		}

		enemy->SetSurface(this->surface);
		enemy->SetRoomID(this->surface->GetID());

		this->surface->AddEnemy(enemy);

		// WAŻNE:
		// Enemy powstał już po wejściu gracza do pokoju,
		// więc trzeba mu ręcznie powiedzieć, że gracz jest w pokoju.
		enemy->OnPlayerEnteredRoom();

		spdlog::info(
		    "DungeonRoomScript: spawned enemy from '{}' in room '{}'",
		    child->GetName(),
		    GetNode()->GetName()
		);
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
