#pragma once

#include <GameObject.h>
#include <physics/ICollisionReceiver.h>

class ThrowBottle : public GameObject, public Physics::ICollisionReceiver {
public:
	ThrowBottle();
	~ThrowBottle();

	void Awake();
	void Update();

	void Break();

	virtual void OnCollisionEnter(SceneNode* otherNode) override;
	virtual void OnCollisionExit(SceneNode* otherNode) override;
};
