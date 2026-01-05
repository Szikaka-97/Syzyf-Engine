#pragma once

#include <vector>
#include <algorithm>

#include <SceneComponent.h>

class GameObject;
class Scene;

class GameObjectSystemBase : public SceneComponent {
	friend class Scene;
protected:
	GameObjectSystemBase(Scene* scene);
	
	virtual bool ValidObject(GameObject* obj) const = 0;
	virtual void RegisterObject(GameObject* obj) = 0;
	virtual bool TryRegisterObject(GameObject* obj) = 0;
	
	virtual void UnregisterObject(GameObject* obj) = 0;
	virtual void UnregisterObjectForced(GameObject* obj) = 0;
public:
	virtual ~GameObjectSystemBase() = default;
};

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
class GameObjectSystem : public GameObjectSystemBase {
	friend class Scene;
private:
	std::vector<T_GO*> objects;

	protected:
	GameObjectSystem(Scene* scene);
	
	virtual bool ValidObject(GameObject* obj) const;
	virtual void RegisterObject(GameObject* obj);
	virtual bool TryRegisterObject(GameObject* obj);
	
	virtual void UnregisterObject(GameObject* obj);
	virtual void UnregisterObjectForced(GameObject* obj);
public:
	virtual ~GameObjectSystem() = default;
	
	std::vector<T_GO*>* GetAllObjects();
};

#include <Scene.h>

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
void GameObjectSystem<T_GO>::UnregisterObjectForced(GameObject* obj) {
	std::erase(this->objects, obj);
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool GameObjectSystem<T_GO>::ValidObject(GameObject* obj) const {
	return dynamic_cast<T_GO*>(obj) != nullptr;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
GameObjectSystem<T_GO>::GameObjectSystem(Scene* scene):
GameObjectSystemBase(scene) {
	this->objects = scene->FindObjectsOfType<T_GO>();
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
void GameObjectSystem<T_GO>::RegisterObject(GameObject* obj) {
	if (ValidObject(obj)) {
		if (std::find(this->objects.begin(), this->objects.end(), obj) == this->objects.end()) {

			this->objects.push_back((T_GO*) obj);
		}
	}
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool GameObjectSystem<T_GO>::TryRegisterObject(GameObject* obj) {
	if (ValidObject(obj)) {
		if (std::find(this->objects.begin(), this->objects.end(), obj) == this->objects.end()) {
			this->objects.push_back((T_GO*) obj);
		}

		return true;
	}

	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
void GameObjectSystem<T_GO>::UnregisterObject(GameObject* obj) {
	if (ValidObject(obj)) {
		std::erase(this->objects, obj);
	}
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
std::vector<T_GO*>* GameObjectSystem<T_GO>::GetAllObjects() {
	return &this->objects;
}