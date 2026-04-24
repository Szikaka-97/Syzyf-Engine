#include "Player.h"
#include <vector>
#include "Scene.h"
#include "Surface.h"
#include "GameObject.h"

Player::Player() : m_RoomID(0) {
	rooms = GetScene()->FindObjectsOfType<Surface>();  
	spdlog::info("Player found {} rooms in the scene", rooms.size());
}

void Player::Update() {
	if (HasRoomChanged()) {
		CheckPosition();
	}
}

bool Player::HasRoomChanged() {
    for (auto* room : rooms) {
        if (room->GetID() == m_RoomID) {
            if (!room->ContainsPoint(this->GlobalTransform().Position())) {
                spdlog::info("Player left room {}", m_RoomID);
                room->InformExit();
                return true;
            }
        }
    }
    return false;
}

void Player::CheckPosition() {
    for (auto* room : rooms) {
        if (room->ContainsPoint(this->GlobalTransform().Position())) {
            m_RoomID = room->GetID();
            spdlog::info("Player entered room {}", m_RoomID);
            room->InformEnter();
            return;
        }
    }
}