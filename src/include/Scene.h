#pragma once

#include <concepts>
#include <vector>
#include <list>
#include <typeinfo>

#include <Transform.h>
#include <spdlog/spdlog.h>

class GameObject;
class SceneGraphics;
class SceneComponent;
class Light;

typedef void (GameObject::*MessageMethod)();

template<class T>
concept Updateable = requires (T a) {
	{ a.Update() } -> std::same_as<void>;
};

template<class T>
concept Awakeable = requires (T a) {
	{ a.Awake() } -> std::same_as<void>;
};

template<class T>
concept Renderable = requires (T a) {
	{ a.Render() } -> std::same_as<void>;
};

template<class T>
concept Enableable = requires (T a) {
	{ a.Enable() } -> std::same_as<void>;
};

template<class T>
concept Disableable = requires (T a) {
	{ a.Disable() } -> std::same_as<void>;
};

template<class T>
concept DrawsGizmos = requires (T a) {
	{ a.DrawGizmos() } -> std::same_as<void>;
};

class Scene;

class SceneNode {
	friend class Scene;
private:
	SceneNode* parent;
	Scene* const scene;
	std::vector<GameObject*> objects;
	std::vector<SceneNode*> children;
	SceneTransform transform;

	SceneNode(Scene* scene);
	SceneNode() = delete;

	void RecalculateTransform();
public:
	~SceneNode();

	SceneTransform& GetTransform();
	SceneTransform::TransformAccess& LocalTransform();
	SceneTransform::TransformAccess& GlobalTransform();

	Scene* GetScene();

	const std::vector<SceneNode*> GetChildren();
	SceneNode* GetParent();
	void SetParent(SceneNode* newParent);
	bool IsChildOf(const SceneNode* node);

	void MarkDirty();
	void MarkChildrenDirty();
	
	const std::vector<GameObject*> AttachedObjects();
	
	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject(T_Param... params);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	T_GO* GetObject() const;

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryGetObject(T_GO*& found) const;

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	std::vector<T_GO*> GetAllObjects() const;

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	T_GO* GetObjectInChildren() const;

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryGetObjectInChildren(T_GO*& found) const;

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	std::vector<T_GO*> GetAllObjectsInChildren() const;

	void DeleteObject(GameObject* obj);
};

class Scene {
	friend class SceneNode;
private:
	struct MessageReceiver {
		GameObject* objPtr;
		MessageMethod methodPtr;

		void Message();
	};

	std::list<MessageReceiver> updateable;
	std::list<MessageReceiver> renderable;
	std::list<MessageReceiver> drawGizmos;
	std::vector<SceneComponent*> components;
	SceneNode* root;

	SceneGraphics* graphics;

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryCreateAwakeable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Awakeable<T_GO>
	bool TryCreateAwakeable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryCreateUpdateable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Updateable<T_GO>
	bool TryCreateUpdateable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryCreateRenderable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Renderable<T_GO>
	bool TryCreateRenderable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryCreateEnableable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Enableable<T_GO>
	bool TryCreateEnableable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryCreateDisableable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Disableable<T_GO>
	bool TryCreateDisableable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	bool TryCreateDrawingGizmos(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && DrawsGizmos<T_GO>
	bool TryCreateDrawingGizmos(T_GO* object);

	void DeleteObjectInternal(GameObject* obj);
	void DeleteNodeInternal(SceneNode* node);
public:
	Scene();
	SceneNode* CreateNode();
	SceneNode* CreateNode(SceneNode* parent);

	SceneGraphics* GetGraphics();

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* CreateObjectOn(SceneNode* node, T_Param... params);

	void DeleteObject(GameObject* obj);
	void DeleteNode(SceneNode* node);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	std::vector<T_GO*> FindObjectsOfType();

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Updateable<T_GO>
	std::vector<T_GO*> FindObjectsOfType();

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Renderable<T_GO>
	std::vector<T_GO*> FindObjectsOfType();

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Updateable<T_GO> && Renderable<T_GO>
	std::vector<T_GO*> FindObjectsOfType();

	template<class T_SC>
		requires std::derived_from<T_SC, SceneComponent>
	T_SC* GetComponent();

	template<class T_SC>
		requires std::derived_from<T_SC, SceneComponent>
	bool TryGetComponent(T_SC*& component);

	template<class T_SC>
		requires std::derived_from<T_SC, SceneComponent>
	T_SC* GetOrCreateComponent();

	template<class T_SC>
		requires std::derived_from<T_SC, SceneComponent>
	T_SC* AddComponent();

	void Update();
	void Render();
};

#include <GameObject.h>
#include <GameObjectSystem.h>

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* SceneNode::AddObject(T_Param... params) {
	return this->scene->CreateObjectOn<T_GO>(this, params...);
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
T_GO* SceneNode::GetObject() const {
	for (GameObject* obj : this->objects) {
		if (dynamic_cast<T_GO*>(obj)) {
			return (T_GO*) obj;
		}
	}

	return nullptr;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool SceneNode::TryGetObject(T_GO*& found) const {
	T_GO* ptr = GetObject<T_GO>();
	
	if (ptr) {
		found = ptr;
		return true;
	}
	
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
std::vector<T_GO*> SceneNode::GetAllObjects() const {
	std::vector<T_GO*> result;

	for (GameObject* obj : this->objects) {
		T_GO* converted = dynamic_cast<T_GO*>(obj);

		if (converted) {
			result.push_back(converted);
		}
	}

	return result;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
T_GO* SceneNode::GetObjectInChildren() const {
	T_GO* result;

	if (TryGetObject(result)) {
		return result;
	}

	for (const auto& child : this->children) {
		if (child->TryGetObjectInChildren(result)) {
			return result;
		}
	}

	return nullptr;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool SceneNode::TryGetObjectInChildren(T_GO*& found) const {
	if (TryGetObject(found)) {
		return true;
	}

	for (const auto& child : this->children) {
		if (child->TryGetObjectInChildren(found)) {
			return true;
		}
	}

	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
std::vector<T_GO*> SceneNode::GetAllObjectsInChildren() const {
	std::vector<T_GO*> result = GetAllObjects<T_GO>();

	for (const auto& child : this->children) {
		std::vector<T_GO*> partial = child->GetAllObjectsInChildren<T_GO>();

		for (const auto& obj : partial) {
			result.push_back(obj);
		}
	}

	return result;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateAwakeable(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Awakeable<T_GO>
bool Scene::TryCreateAwakeable(T_GO* object) {
	object->Awake();

	return true;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateUpdateable(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Updateable<T_GO>
bool Scene::TryCreateUpdateable(T_GO* object) {
	this->updateable.push_back({ object, reinterpret_cast<MessageMethod>(&T_GO::Update) });

	return true;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateEnableable(T_GO* object) {
	object->onEnable = nullptr;

	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Enableable<T_GO>
bool Scene::TryCreateEnableable(T_GO* object) {
	object->onEnable = reinterpret_cast<MessageMethod>(&T_GO::Enable);
	
	object->Enable();

	return true;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateDisableable(T_GO* object) {
	object->onDisable = nullptr;

	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Disableable<T_GO>
bool Scene::TryCreateDisableable(T_GO* object) {
	object->onDisable = reinterpret_cast<MessageMethod>(&T_GO::Disable);
	
	return true;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateRenderable(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Renderable<T_GO>
bool Scene::TryCreateRenderable(T_GO* object) {
	this->renderable.push_back({ object, reinterpret_cast<MessageMethod>(&T_GO::Render) });

	return true;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateDrawingGizmos(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && DrawsGizmos<T_GO>
bool Scene::TryCreateDrawingGizmos(T_GO* object) {
	this->drawGizmos.push_back({ object, reinterpret_cast<MessageMethod>(&T_GO::DrawGizmos) });

	return true;
}

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* Scene::CreateObjectOn(SceneNode* node, T_Param... params) {
	alignas(T_GO) unsigned char* dataBuf = new unsigned char[sizeof(T_GO)];
	memset(dataBuf, 0, sizeof(T_GO));
	volatile T_GO* bufAsObjPtr = reinterpret_cast<T_GO*>(dataBuf);

	bufAsObjPtr->node = node;

	T_GO* created = new(const_cast<T_GO*>(bufAsObjPtr)) T_GO(params...);

	created->node = node;
	
	node->objects.push_back(created);

	TryCreateAwakeable(created);
	TryCreateEnableable(created);
	TryCreateUpdateable(created);
	TryCreateRenderable(created);
	TryCreateDrawingGizmos(created);

	for (SceneComponent* component : this->components) {

		GameObjectSystemBase* sys = dynamic_cast<GameObjectSystemBase*>(component);

		if (sys && sys->ValidObject(created)) {
			sys->RegisterObject(created);
		}
	}

	created->enabled = true;

	return created;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
std::vector<T_GO*> Scene::FindObjectsOfType() {
	return this->root->GetAllObjectsInChildren<T_GO>();
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Updateable<T_GO>
std::vector<T_GO*> Scene::FindObjectsOfType() {
	std::vector<T_GO*> result;

	for (const auto& updateableObj : this->updateable) {
		T_GO* objPtr = dynamic_cast<T_GO*>(updateableObj.objPtr);

		if (objPtr) {
			result.push_back(objPtr);
		}
	}

	return result;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Renderable<T_GO>
std::vector<T_GO*> Scene::FindObjectsOfType() {
	std::vector<T_GO*> result;

	for (const auto& renderableObj : this->renderable) {
		T_GO* objPtr = dynamic_cast<T_GO*>(renderableObj.objPtr);

		if (objPtr) {
			result.push_back(objPtr);
		}
	}

	return result;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Updateable<T_GO> && Renderable<T_GO>
std::vector<T_GO*> Scene::FindObjectsOfType() {
	std::vector<T_GO*> result;

	if (this->updateable.size() <= this->renderable.size()) {
		for (const auto& updateableObj : this->updateable) {
			T_GO* objPtr = dynamic_cast<T_GO*>(updateableObj.objPtr);

			if (objPtr) {
				result.push_back(objPtr);
			}
		}
	}
	else {
		for (const auto& renderableObj : this->renderable) {
			T_GO* objPtr = dynamic_cast<T_GO*>(renderableObj.objPtr);

			if (objPtr) {
				result.push_back(objPtr);
			}
		}
	}

	return result;
}

template<class T_SC>
	requires std::derived_from<T_SC, SceneComponent>
T_SC* Scene::GetComponent() {
	for (SceneComponent* component : this->components) {
		
		if (typeid(T_SC) == typeid(component)) {
			T_SC* result = dynamic_cast<T_SC*>(component);

			return result;
		}
	}

	return nullptr;
}

template<class T_SC>
	requires std::derived_from<T_SC, SceneComponent>
bool Scene::TryGetComponent(T_SC*& component) {
	component = GetComponent<T_SC>();

	return component != nullptr;
}

template<class T_SC>
	requires std::derived_from<T_SC, SceneComponent>
T_SC* Scene::GetOrCreateComponent() {
	T_SC* component = GetComponent<T_SC>();

	if (component == nullptr) {
		return AddComponent<T_SC>();
	}
}

template<class T_SC>
	requires std::derived_from<T_SC, SceneComponent>
T_SC* Scene::AddComponent() {
	T_SC* component = GetComponent<T_SC>();

	if (component == nullptr) {
		component = new T_SC(this);
		
		this->components.push_back(component);

		for (int i = this->components.size() - 2; i >= 0; i--) {
			if (this->components[i]->Order() > this->components[i + 1]->Order()) {
				std::swap(this->components[i], this->components[i + 1]);
			}
		}
	}
	
	return component;
}