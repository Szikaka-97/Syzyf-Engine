#pragma once

#include <concepts>
#include <vector>
#include <list>

#include <Transform.h>
#include <Graphics.h>
#include <spdlog/spdlog.h>

class GameObject;
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
	std::list<const Light*> sceneLights;
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
	bool TryAddLight(T_GO* object);

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

	const std::list<const Light*> GetSceneLights() const;

	void Update();
	void Render();
};

#include <GameObject.h>

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
bool Scene::TryAddLight(T_GO* object) {
	return false;
}

template<>
bool Scene::TryAddLight<Light>(Light* object);

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
	TryAddLight(created);

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