#pragma once

#include <concepts>
#include <nlohmann/json_fwd.hpp>
#include <vector>
#include <queue>
#include <Serialized.h>

#include <spdlog/spdlog.h>

#include <Transform.h>
#include <Resources.h>
#include <Messaging.h>

class GameObject;
class InputSystem;
class SceneGraphics;
class SceneComponent;
class Light;

#ifdef _WIN32
#include <Windows.h>
#endif

class Scene;

class SceneNode {
	friend class Scene;
	friend class SceneTransform;
private:
	serialized SceneNode* parent;

	serialized int id;
	serialized std::string name;

	serialized uint8_t disabledState;
	serialized uint8_t layer;

	serialized Scene* const scene;
	std::vector<GameObject*> objects;

	serialized std::vector<SceneNode*> children;
	SceneTransform transform;

	SceneNode(Scene* scene);
	SceneNode() = delete;

	void RecalculateTransform();
public:
	~SceneNode();

	SceneTransform& GetTransform();
	SceneTransform::TransformAccess& LocalTransform();
	SceneTransform::TransformAccess& GlobalTransform();

	int GetID() const;

	std::string GetName() const;
	void SetName(const std::string& name);

	Scene* GetScene();

	bool IsEnabled() const;

	bool EnabledSelf() const;
	void SetEnabled(bool value);

	const std::vector<SceneNode*> GetChildren();
	SceneNode* GetParent();
	void SetParent(SceneNode* newParent);
	bool IsChildOf(const SceneNode* node);

	SceneNode* FindNode(const fs::path& nodePath) const;
	bool TryFindNode(const fs::path& nodePath, SceneNode** node) const;

	void MarkDirty();
	void MarkChildrenDirty();
	
	uint8_t GetLayer() const;
	bool CheckLayerMask(uint32_t layerMask);
	void SetLayer(uint8_t layer);

	const std::vector<GameObject*> AttachedObjects();
	
	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject(T_Param&&... params);

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

	static void operator delete(SceneNode* ptr, std::destroying_delete_t);

	json Serialize() const;
	void Deserialize(const json& data);
};

class Scene {
	friend class SceneNode;
	friend class GameObject;
	friend class MessagingHelpers;
public:
	serialized std::string name = "";

	int nextSceneNodeID;
	int nextGameObjectID;

	ResourceDatabase resources;

	std::vector<SceneComponent*> components;
	MessageTree messageTree;
	serialized SceneNode* root;

	InputSystem* inputSystem;
	SceneGraphics* graphics;

	std::queue<GameObject*> deletedObjectsQueue;
	std::queue<SceneNode*> deletedNodesQueue;

	void DeleteObjectInternal(GameObject* obj);
	void DeleteNodeInternal(SceneNode* node);
	void SetNodeEnabledInternal(SceneNode* node, bool enabled);
	void SetGameObjectEnabledInternal(GameObject* obj, bool enabled);
	void ChangeNodeParentInternal(SceneNode* node, SceneNode* newParent);
	void SetNodeEnabledInTreeInternal(SceneNode* node, bool enabled);

	void DeserializeGameObject(SceneNode* node, json data);

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	void AddGameObjectInternal(SceneNode* node, T_GO* obj);

	void AddObjectToSystems(GameObject* obj);
public:
	static Scene* CreateStandaloneScene();

	Scene();

	~Scene();

	SceneNode* CreateNode();
	SceneNode* CreateNode(SceneNode* parent);
	SceneNode* CreateNode(const std::string& name);
	SceneNode* CreateNode(SceneNode* parent, const std::string& name);

	ResourceDatabase* Resources();

	InputSystem* Input();
	SceneGraphics* GetGraphics();

	SceneNode* GetRootNode();

	SceneNode* FindNode(const fs::path& nodePath) const;
	bool TryFindNode(const fs::path& nodePath, SceneNode** node) const;

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* CreateObjectOn(SceneNode* node, T_Param&&... params);

	void DeleteObject(GameObject* obj);
	void DeleteNode(SceneNode* node);

    void FlushQueues();

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
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

	template<class T_SC>
		requires std::derived_from<T_SC, SceneComponent>
	void RemoveComponent();

	void QueueDelete(SceneNode* node);
	void QueueDelete(GameObject* object);

	void Update();
	void Render();
	void DrawGizmos();
	void OnEnable();
	void OnDisable();

	void DrawImGui();

	static void operator delete(Scene* ptr, std::destroying_delete_t);

	void Deserialize(const nlohmann::json& json_node);
	nlohmann::json Serialize() const;
};

#include <GameObject.h>
#include <SceneComponent.h>

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* SceneNode::AddObject(T_Param&&... params) {
	return this->scene->CreateObjectOn<T_GO>(this, std::forward<T_Param>(params)...);
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


template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
void Scene::AddGameObjectInternal(SceneNode* node, T_GO* obj) {
	obj->node = node;
	
	node->objects.push_back(obj);

	this->messageTree.AddMessageReceiver(obj);

	AddObjectToSystems(obj);

	obj->id = this->nextGameObjectID++;

	obj->enabled = true;

	this->messageTree.MessageObject<Message::Awake>(obj);
	this->messageTree.MessageObject<Message::OnEnable>(obj);
}


template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* Scene::CreateObjectOn(SceneNode* node, T_Param&&... params) {
	alignas(T_GO) unsigned char* dataBuf = new unsigned char[sizeof(T_GO)];
	memset(dataBuf, 0, sizeof(T_GO));
	volatile T_GO* bufAsObjPtr = reinterpret_cast<T_GO*>(dataBuf);

	bufAsObjPtr->node = node;

	T_GO* created = new(const_cast<T_GO*>(bufAsObjPtr)) T_GO(std::forward<T_Param>(params)...);
	
	AddGameObjectInternal<T_GO>(node, created);

	return created;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
std::vector<T_GO*> Scene::FindObjectsOfType() {
	return this->root->GetAllObjectsInChildren<T_GO>();
}

template<class T_SC>
	requires std::derived_from<T_SC, SceneComponent>
T_SC* Scene::GetComponent() {
	for (SceneComponent* component : this->components) {
		T_SC* result = dynamic_cast<T_SC*>(component);
		if (result != nullptr) {
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

template<class T_SC>
	requires std::derived_from<T_SC, SceneComponent>
void Scene::RemoveComponent() {
	T_SC* component = GetComponent<T_SC>();

	if (component != nullptr) {
		std::erase(this->components, component);

		delete component;
	}
}
