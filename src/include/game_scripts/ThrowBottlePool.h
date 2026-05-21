#pragma once

#include <vector>

#include <SceneComponent.h>

class SceneNode;
class ThrowBottle;
class Mesh;
class Material;

class ThrowBottlePool : public SceneComponent {
private:
	struct BottleInstance {
		SceneNode* bottle;
		int id;

		inline bool operator<(const BottleInstance& other) const {
			return this->id < other.id;
		}
	};

	SceneNode* bottlesPoolRoot;
	Mesh* bottleMesh;
	Material* bottleMaterial;

	std::vector<BottleInstance> allInstances;
public:
	int capacity;

	ThrowBottlePool(Scene *scene);

	SceneNode* RequestThrowBottle();
	void ReturnBottleToPool(ThrowBottle* bottle);
};