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
   // spdlog::warn("PLayer: Player is in room {}", m_RoomID);
	if (HasRoomChanged()) {
		CheckPosition();
	}
}

bool Player::HasRoomChanged() {
    for (auto* room : rooms) {
        if (room->GetID() == m_RoomID) {
            if (!room->ContainsPoint(this->GlobalTransform().Position(), 0.2f)) {
                room->InformExit();
                return true;
            }
            return false;
        }
    }
    return false;
}

void Player::CheckPosition() {
    for (auto* room : rooms) {
        if (room->GetID() == m_RoomID && room->ContainsPoint(this->GlobalTransform().Position(), 0.0f))
            return; 
    }
    for (auto* room : rooms) {
        if (room->ContainsPoint(this->GlobalTransform().Position(), 0.0f)) {
            m_RoomID = room->GetID();
            room->InformEnter();
            return;
        }
    }
}