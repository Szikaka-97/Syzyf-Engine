#pragma once

#include <cstddef>
#include <iterator>
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
	struct iterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = T_GO*;
		using pointer = value_type*;
		using reference = value_type&;
	private:
		pointer valuePtr;
		iterator* guardPtr;
		bool includeInactive;
	public:
		iterator(pointer valuePtr, iterator* guardPtr, bool includeInactive):
		valuePtr(valuePtr),
		guardPtr(guardPtr),
		includeInactive(includeInactive) { }

		friend bool operator== (const iterator& a, const iterator& b) { return a.valuePtr == b.valuePtr; };
		friend bool operator!= (const iterator& a, const iterator& b) { return a.valuePtr != b.valuePtr; };     

		reference operator*() const { return *this->valuePtr; }
		pointer operator->() { return this->valuePtr; }

		iterator& operator++() {
			do {
				this->valuePtr++;
			} while(*this != *this->guardPtr && !this->includeInactive && !(*this->valuePtr)->IsEnabled());

			return *this;
		}

		iterator operator++(int) {
			auto tmp = *this;

			++(*this);

			return tmp;
		}
	};

	struct ForLoop {
	private:
		GameObjectSystem<T_GO>* source;
		iterator current;
		iterator guard;
		bool includeInactive;
	public:
		ForLoop(GameObjectSystem<T_GO>* source, bool includeInactive):
		source(source),
		guard(&source->objects[source->objects.size()], &guard, includeInactive),
		current(&source->objects[0], &guard, includeInactive),
		includeInactive(includeInactive) { }

		iterator begin();
		iterator end();
	};

	virtual ~GameObjectSystem() = default;
	
	std::vector<T_GO*>* GetAllObjects();

	ForLoop IterateObjects(bool includeInactive = false);
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

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
GameObjectSystem<T_GO>::ForLoop GameObjectSystem<T_GO>::IterateObjects(bool includeInactive) {
	return ForLoop(this, includeInactive);
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
GameObjectSystem<T_GO>::iterator GameObjectSystem<T_GO>::ForLoop::begin() {
	while (this->current != this->guard && !this->includeInactive && !(*this->current)->IsEnabled()) {
		++this->current;
	}

	return this->current;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
GameObjectSystem<T_GO>::iterator GameObjectSystem<T_GO>::ForLoop::end() {
	return this->guard;
}