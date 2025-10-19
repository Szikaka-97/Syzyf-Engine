#pragma once

#include <concepts>
#include <vector>
#include <list>

#include <Transform.h>

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
	SceneNode* root;

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
	bool TryCreateEnableable(T_GO* object);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject> && Enableable<T_GO>
	bool TryCreateEnableable(T_GO* object);
public:
	Scene();
	SceneNode* CreateNode();
	SceneNode* CreateNode(SceneNode* parent);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	T_GO* CreateObjectOn(SceneNode* node);

	void Update();
	void Render();
};