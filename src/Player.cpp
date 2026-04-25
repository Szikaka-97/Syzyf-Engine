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

// Player.cpp – poprawione metody

bool Player::HasRoomChanged() {
    for (auto* room : rooms) {
        if (room->GetID() == m_RoomID) {
            // Gracz opuszcza pokój tylko wtedy, gdy jest wyraŸnie poza nim (margines 0.2)
            if (!room->ContainsPoint(this->GlobalTransform().Position(), 0.2f)) {
               // spdlog::info("Player left room {}", m_RoomID);
                room->InformExit();
                return true;
            }
            // Jeœli wci¹¿ jesteœmy w tym pokoju (nawet z marginesem), nie zmieniamy
            return false;
        }
    }
    return false;
}

void Player::CheckPosition() {
    // Najpierw sprawdŸ, czy nadal jesteœmy w bie¿¹cym pokoju (z tolerancj¹)
    for (auto* room : rooms) {
        if (room->GetID() == m_RoomID && room->ContainsPoint(this->GlobalTransform().Position(), 0.0f))
            return; // nic nie zmieniaj
    }
    // Dopiero gdy nie ma bie¿¹cego pokoju, szukamy nowego
    for (auto* room : rooms) {
        if (room->ContainsPoint(this->GlobalTransform().Position(), 0.0f)) {
            m_RoomID = room->GetID();
          //  spdlog::info("Player entered room {}", m_RoomID);
            room->InformEnter();
            return;
        }
    }
    // Jeœli nie znaleziono ¿adnego pokoju, ustaw na -1 (opcjonalnie)
    // m_RoomID = -1;
}