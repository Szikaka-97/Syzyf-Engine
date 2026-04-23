#include "Player.h"
#include <vector>
#include "Scene.h"
#include "Surface.h"
#include "GameObject.h"

Player::Player() : m_RoomID(0) {
	rooms = GetScene()->FindObjectsOfType<Surface>();  
}

void Player::Update() {
	if (HasRoomChanged()) {
		CheckPosition();
	}
}

bool Player::HasRoomChanged() {
	//std::vector<SceneNode*> rooms = GetScene()->FinfObjectsOfType<Surface>;
	for (auto room : rooms) {
		if (room->GetID()==m_RoomID) { //check if its my room 
			if (!room->IsOnSurface(this->GlobalTransform().Position())) {
				//my room has changed
				spdlog::error("Player left room {}, checking for new room", m_RoomID);
				room->InformExit();
				return true;
			}
		}
	}
	return false;
}

void Player::CheckPosition() {
	//std::vector<SceneNode*> rooms = GetScene()->FinfObjectsOfType<Surface>;
	for (auto room : rooms) {
		if (room->IsOnSurface(this->GlobalTransform().Position())) { //check if im in this room 
			this->m_RoomID = room->GetID();
			spdlog::error("Player entered room {}, informing room and enemies", m_RoomID);
			room->InformEnter();
			return;
		}
	}
}