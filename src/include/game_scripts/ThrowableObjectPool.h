#pragma once

#include <vector>

#include <SceneComponent.h>

class SceneNode;
class ThrowableObject;
class Mesh;
class Material;

class ThrowableObjectPool : public SceneComponent {
private:
	struct ThrowableInstance {
		SceneNode* bottle;
		int id;

		inline bool operator<(const ThrowableInstance& other) const {
			return this->id < other.id;
		}
	};

	SceneNode* bottlesPoolRoot;
	Mesh* bottleMesh;
	Material* bottleMaterial;

	std::vector<ThrowableInstance> allInstances;
public:
	int capacity;

	ThrowableObjectPool(Scene *scene);

	SceneNode* RequestThrowableObject();
	void ReturnBottleToPool(ThrowableObject* bottle);
};