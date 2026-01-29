#pragma once

#include <concepts>
#include <typeinfo>

#include <Scene.h>

class GameObject {
	friend class Scene;
private:
	int id;
	const std::type_info* runtimeTypeInfo;
	bool enabled;
	MessageMethod onEnable;
	MessageMethod onDisable;
protected:
	SceneNode* node;
	
	SceneTransform& GetTransform() const;
	SceneTransform::TransformAccess& GlobalTransform() const;
	SceneTransform::TransformAccess& LocalTransform() const;
	SceneNode* GetNode() const;
	Scene* GetScene() const;
public:
	virtual ~GameObject();

	int GetID() const;
	std::string GetName() const;

	SceneTransform& GetTransform();
	SceneTransform::TransformAccess& GlobalTransform();
	SceneTransform::TransformAccess& LocalTransform();
	SceneNode* GetNode();
	Scene* GetScene();

	bool IsEnabled() const;
	void SetEnabled(bool enabled);

	template<class T_GO>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject() const;

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* AddObject(T_Param... params) const;

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	T_GO* GetObject() const;

	template<class T_GO, typename... T_Param>
		requires std::derived_from<T_GO, GameObject>
	bool TryGetObject(T_GO*& target) const;
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

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
T_GO* GameObject::GetObject() const {
	return this->node->GetObject<T_GO>();
}

template<class T_GO, typename... T_Param>
	requires std::derived_from<T_GO, GameObject>
bool GameObject::TryGetObject(T_GO*& target) const {
	return this->node->TryGetObject<T_GO>(target);
}

// template <class T_Required>
// 	requires std::derived_from<T_Required, GameObject>
// class Requires { };