#pragma once

#include <GameObject.h>
#include <TimeSystem.h>

#include <game_scripts/PickableItem.h>


class Text3D;

class GateKey : public PickableItem {
public:
	GateKey() = default;

	virtual void OnPickUp() override;
};

class BaseLights : public GameObject {
private:
	std::vector<Light*> lights;
	std::vector<float> baseIntensities;
public:
	BaseLights() = default;

	void Awake();

	void Update();
};

class BaseScript : public GameObject { // Move to own file later
private:
	SceneNode* gate;
	glm::vec3 exitVolume;
	SceneNode* key;
	bool gateLowering = false;
public:
	BaseScript() = default;

	void Awake();

	void Update();
};

class BaseTutorialManager : public GameObject {
private:
	Text3D* tutorialText;
	float timePoint;
	bool playerStartedMoving = false;
	bool playerReachedDoors = false;
	bool playerMovedAwayFromDoors = false;
	bool playerFoundKey = false;
	bool playerGotCloseToKey = false;

	inline float TimeSincePoint() const {
		return Time::Current() - timePoint;
	}
public:
	BaseTutorialManager() = default;

	void Awake();

	void Update();
};

class BaseExitToTutorialThrowingRoom : public GameObject {
private:
	bool sceneRequested = false;
	glm::vec3 triggerPosition = glm::vec3(1.6686f, 0.0f, 20.0f);
	float triggerRadius = 2.5f;
public:
	BaseExitToTutorialThrowingRoom() = default;

	void Awake();

	void Update();
};
