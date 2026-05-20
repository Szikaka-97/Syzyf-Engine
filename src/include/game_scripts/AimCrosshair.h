#pragma once

#include <GameObject.h>
#include <Debug.h>

class AimCrosshair : public GameObject, public ImGuiDrawable {
private:
	glm::vec3 initialArrowPos;

	SceneNode* arrow = nullptr;
	SceneNode* bottle = nullptr;
public:
	void Awake();
	void Update();

	virtual void DrawImGui() override;
};