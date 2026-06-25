#include <game_scripts/enemies/EnemySword.h>

#include <game_scripts/PlayerController.h>

#include <GltfScene.h>


EnemySword::EnemySword() {
	//SceneNode* enemyModel = GetScene()->resources.Get<GltfScene>("./res/models/sword.glb")->Instantiate(GetScene(), GetNode(), "sword");
	//m_Owner      = owner;
	spdlog::error("init sword");
	m_PlayerNode = GetScene()->FindNode("PlayerController");

	myNode = GetNode();
}

void EnemySword::Update() {
	if (m_HasHit || !m_PlayerNode || !myNode) return;

	glm::vec3 segPos    = myNode->GlobalTransform().Position();
	glm::vec3 playerPos = m_PlayerNode->GlobalTransform().Position();

	if (glm::distance(segPos, playerPos) <= m_HitRadius) {
		m_HasHit = true;

		auto* pc = m_PlayerNode->GetObject<PlayerController>();
		if (pc) pc->TakeDamage(15);

	}
}

void EnemySword::OnCollisionEnter(SceneNode* other)  {
	if (other == GetNode()) return;
	auto* player = other->GetObject<PlayerController>();
	if (player) {
		player->TakeDamage(10); // Arbitrary damage value
		spdlog::info("EnemySword: hit player for 10 damage");
	}
}
