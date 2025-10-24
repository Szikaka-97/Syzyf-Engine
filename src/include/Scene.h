#pragma once

#include <concepts>
#include <vector>
#include <list>

#include <Transform.h>
#include <Graphics.h>
#include <spdlog/spdlog.h>

class GameObject;

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
	Transform transform;

	SceneNode(Scene* scene);
	SceneNode() = delete;

	void RecalculateTransform();
public:
	Transform& GetTransform();
	Transform::TransformAccess& LocalTransform();
	Transform::TransformAccess& GlobalTransform();

	Scene* GetScene();

	const std::vector<SceneNode*> GetChildren();
	SceneNode* GetParent();
	void SetParent(SceneNode* newParent);
	bool IsChildOf(const SceneNode* node);

	void MarkDirty();
	void MarkChildrenDirty();
	
	const std::vector<GameObject*> AttachedObjects();
	
	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject();

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject(T_Param... params);
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
public:
	Scene();
	SceneNode* CreateNode();
	SceneNode* CreateNode(SceneNode* parent);

	SceneGraphics* GetGraphics();

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	T_GO* CreateObjectOn(SceneNode* node);

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* CreateObjectOn(SceneNode* node, T_Param... params);

	void Update();
	void Render();
};

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
T_GO* SceneNode::AddObject() {
	return this->scene->CreateObjectOn<T_GO>(this);
}

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* SceneNode::AddObject(T_Param... params) {
	return this->scene->CreateObjectOn<T_GO>(this, params...);
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject>
bool Scene::TryCreateAwakeable(T_GO* object) {
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Awakeable<T_GO>
bool Scene::TryCreateAwakeable(T_GO* object) {
#warning TODO
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
	return false;
}

template<class T_GO>
	requires std::derived_from<T_GO, GameObject> && Enableable<T_GO>
bool Scene::TryCreateEnableable(T_GO* object) {
#warning TODO
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
T_GO* Scene::CreateObjectOn(SceneNode* node) {
	T_GO* created = new T_GO();
	created->node = node;

	TryCreateAwakeable(created);
	TryCreateEnableable(created);
	TryCreateUpdateable(created);
	TryCreateRenderable(created);

	return created;
}

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* Scene::CreateObjectOn(SceneNode* node, T_Param... params) {
	T_GO* created = new T_GO(params...);
	created->node = node;

	TryCreateAwakeable(created);
	TryCreateEnableable(created);
	TryCreateUpdateable(created);
	TryCreateRenderable(created);

	return created;
}