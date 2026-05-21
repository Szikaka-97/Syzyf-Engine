#include <game_scripts/ThrowBottle.h>

#include <physics/Body.h>
#include <physics/System.h>
#include <game_scripts/PlayerController.h>
#include <Formatters.h>


ThrowBottle::ThrowBottle() { }

ThrowBottle::~ThrowBottle() { }

void ThrowBottle::Awake() {

}

void ThrowBottle::Update() {
	if (GlobalTransform().Position().y < -10) {
		delete this->GetNode();
	}

}

void ThrowBottle::OnCollisionEnter(SceneNode* otherNode) {
	PlayerController* _;
	if (!otherNode->TryGetObject<PlayerController>(_)) {
		delete this->GetNode();
	}
}

void ThrowBottle::OnCollisionExit(SceneNode* otherNode) { }