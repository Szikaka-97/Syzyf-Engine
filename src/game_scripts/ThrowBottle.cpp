#include <game_scripts/ThrowBottle.h>

#include <physics/Body.h>
#include <physics/System.h>
#include <game_scripts/PlayerController.h>
#include <Formatters.h>
#include <game_scripts/ThrowBottlePool.h>
#include <game_scripts/bottle_effects/BottleEffect.h>

ThrowBottle::ThrowBottle() { }

ThrowBottle::~ThrowBottle() { }

void ThrowBottle::Awake() {

}

void ThrowBottle::Update() {
	if (GlobalTransform().Position().y < -10) {
		delete this->GetNode();
	}

}

void ThrowBottle::Break() {
	for (BottleEffect* obj : GetNode()->GetAllObjectsInChildren<BottleEffect>()) {
		obj->Effect();
	}
}

void ThrowBottle::OnCollisionEnter(SceneNode* otherNode) {
	PlayerController* _;
	if (!otherNode->TryGetObject<PlayerController>(_)) {
		Break();

		GetScene()->GetComponent<ThrowBottlePool>()->ReturnBottleToPool(this);
	}
}

void ThrowBottle::OnCollisionExit(SceneNode* otherNode) { }