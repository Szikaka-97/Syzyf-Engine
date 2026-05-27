#include <game_scripts/DungeonGenerator.h>

#include <numeric>

#include <GltfScene.h>
#include <Formatters.h>
#include <imgui.h>

SceneNode* DungeonGenerator::PlaceRoom() {
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
		delete room;
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

void DungeonGenerator::Render() {
	static int roomCounter = 0;

	if (!this->roomsToSpawn.empty()) {
		auto room = this->roomsToSpawn.front();
		this->roomsToSpawn.pop();

		GltfScene* roomPrefab = nullptr;

		if (room.position == glm::vec2(0, 0)) {
			roomPrefab = ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Room Start.glb");
		}
		else if (room.type == RoomShape::Corridor) {
			roomPrefab = ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Room Corridor.glb");
		}
		else if (room.type == RoomShape::DeadEnd) {
			roomPrefab = ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Room DeadEnd.glb");
		}
		else if (room.type == RoomShape::TShape) {
			roomPrefab = ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Room T.glb");
		}
		else if (room.type == RoomShape::Corner) {
			roomPrefab = ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Room L.glb");
		}
		else if (room.type == RoomShape::Cross) {
			roomPrefab = ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Room Cross.glb");
		}

		roomCounter++;

		if (roomPrefab) {
			spdlog::info("Room coords: {}", room.position);

			SceneNode* spawnedRoom = roomPrefab->Instantiate(GetScene(), GetNode(), std::format("Room {}", roomCounter));
			
			spawnedRoom->GlobalTransform().Position() = GlobalTransform().Position() + glm::vec3(room.position.y, 0, room.position.x) * glm::vec3(this->gridSize);
			spawnedRoom->GlobalTransform().Rotation() = glm::vec3(0, glm::radians(-90.0f * room.orientation), 0);

			this->dungeonRooms.push_back(spawnedRoom);
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