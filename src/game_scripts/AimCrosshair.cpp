#include <game_scripts/AimCrosshair.h>

#include <TimeSystem.h>

void AimCrosshair::Awake() {
	this->bottle = GetNode()->FindNode("Bottle");
	this->arrow = this->bottle->FindNode("Arrow");

	this->initialArrowPos = this->arrow->LocalTransform().Position();

	assert(this->arrow);
	assert(this->bottle);
}

void AimCrosshair::Update() {
	this->arrow->LocalTransform().Position() = initialArrowPos + glm::vec3(0, glm::sin(Time::Current() * 2) * 0.1f, 0);
	this->bottle->LocalTransform().Rotation() *= glm::angleAxis(Time::Delta() * 1.2f, glm::vec3(0, 1, 0));
}

void AimCrosshair::DrawImGui() {

}