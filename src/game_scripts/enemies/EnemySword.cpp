#include "./include/game_scripts/enemies/EnemySword.h"
#include "./include/game_scripts/PlayerController.h"
#include <Player.h>

void EnemySword::Update() {
	// For testing: make the sword follow the enemy's hand position
	/*if (auto* parentNode = GetNode()->Parent()) {
		GlobalTransform() = parentNode->GlobalTransform();
	}*/
}

void EnemySword::OnCollisionEnter(SceneNode* other)  {
	if (other == GetNode()) return;
	auto* player = other->GetObject<PlayerController>();
	if (player) {
		player->TakeDamage(10); // Arbitrary damage value
		spdlog::info("EnemySword: hit player for 10 damage");
	}
}
