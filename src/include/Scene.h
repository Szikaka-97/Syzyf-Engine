#pragma once

#include <concepts>
#include <vector>
#include <typeinfo>
#include <queue>

#include <spdlog/spdlog.h>

#include <Transform.h>
#include <Resources.h>
#include <Messaging.h>

class GameObject;
class InputSystem;
class SceneGraphics;
class SceneComponent;
class Light;

class Scene;

class SceneNode {
	friend class Scene;
private:
	SceneNode* parent;

	int id;
	std::string name;

	bool enabled;
	uint8_t layer;

	Scene* const scene;
	std::vector<GameObject*> objects;
	std::vector<Scene*> attachedScenes;

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

	int GetID() const;

	std::string GetName() const;
	void SetName(const std::string& name);

	Scene* GetScene();

	bool IsEnabled() const;
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

	void AttachScene(Scene* scene);
	void DetachScene(Scene* scene);

	std::vector<Scene*> GetAttachedScenes() const;

	static void operator delete(SceneNode* ptr, std::destroying_delete_t);
};

class Scene : public MessageReceiver{
	friend class SceneNode;
	friend class GameObject;
private:
	int nextSceneNodeID;
	int nextGameObjectID;

	ResourceDatabase resources;

	std::vector<SceneComponent*> components;
	MessageTree messageTree;
	SceneNode* root;

	InputSystem* inputSystem;
	SceneGraphics* graphics;

	std::queue<MessageReceiver*> deletedReceiversQueue;
	std::queue<SceneNode*> deletedNodesQueue;

	void DeleteObjectInternal(GameObject* obj);
	void DeleteNodeInternal(SceneNode* node);
	void SetNodeEnabledInternal(SceneNode* node, bool enabled);
	void SetGameObjectEnabledInternal(GameObject* obj, bool enabled);
	void ChangeNodeParentInternal(SceneNode* node, SceneNode* newParent);
	void AttachSceneToNodeInternal(SceneNode* node, Scene* scene);
	void DetachSceneFromNodeInternal(SceneNode* node, Scene* scene);
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
	T_GO* CreateObjectOn(SceneNode* node, T_Param... params);

	void DeleteObject(GameObject* obj);
	void DeleteNode(SceneNode* node);

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
	void QueueDelete(Scene* scene);

	void Update();
	void Render();
	void DrawGizmos();
	void OnEnable();
	void OnDisable();

	void DrawImGui();

	static void operator delete(Scene* ptr, std::destroying_delete_t);
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

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* Scene::CreateObjectOn(SceneNode* node, T_Param... params) {
	alignas(T_GO) unsigned char* dataBuf = new unsigned char[sizeof(T_GO)];
	memset(dataBuf, 0, sizeof(T_GO));
	volatile T_GO* bufAsObjPtr = reinterpret_cast<T_GO*>(dataBuf);

	bufAsObjPtr->node = node;

	T_GO* created = new(const_cast<T_GO*>(bufAsObjPtr)) T_GO(params...);
	
	created->node = node;
	created->runtimeTypeInfo = &typeid(T_GO);
	
	node->objects.push_back(created);

	this->messageTree.AddMessageReceiver(created, node);

	for (SceneComponent* component : this->components) {
		GameObjectSystemBase* sys = dynamic_cast<GameObjectSystemBase*>(component);

		if (sys && sys->ValidObject(created)) {
			sys->RegisterObject(created);
		}
	}

	created->id = this->nextGameObjectID++;

	created->enabled = true;

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
