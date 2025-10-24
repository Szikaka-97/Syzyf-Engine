#pragma once

#include <concepts>

#include <Scene.h>

class GameObject {
	friend class Scene;
protected:
	SceneNode* node;
	Transform& GetTransform() const;
	Transform::TransformAccess& GlobalTransform() const;
	Transform::TransformAccess& LocalTransform() const;
	SceneNode* GetNode() const;
	Scene* GetScene() const;
public:
	Transform& GetTransform();
	Transform::TransformAccess& GlobalTransform();
	Transform::TransformAccess& LocalTransform();
	SceneNode* GetNode();
	Scene* GetScene();

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject() const;

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject(T_Param... params) const;
};

template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
T_GO* GameObject::AddObject() const {
	return this->node->AddObject<T_GO>();
}

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* GameObject::AddObject(T_Param... params) const {
	return this->node->AddObject<T_GO>(params...);
}

// template <class T_Required>
// 	requires std::derived_from<T_Required, GameObject>
// class Requires { };