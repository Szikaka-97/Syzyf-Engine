#pragma once

#include <GameObject.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class AimingAid : public GameObject {
public:
	glm::vec3 forwardAxis;
	glm::vec3 upAxis;

	SceneNode* stretchPart;
	SceneNode* crosshair;

	void PointAt(glm::vec3 point) {
		GlobalTransform().Rotation() = glm::quatLookAt(point - GlobalTransform().Position(), glm::vec3(0, 1, 0)) * glm::quatLookAt(forwardAxis, upAxis);
	}

	void SetStretch(float amount) {
		if (amount < 0) {
			// stretchPart->SetEnabled(false);
		}
		else {
			// stretchPart->SetEnabled(true);
			//TODO
		}
	}

	void SetCrosshairPosition(glm::vec3 pos) {
		this->crosshair->GlobalTransform().Position() = pos;
	}
};