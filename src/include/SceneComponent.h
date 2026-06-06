#pragma once

#include <Serialized.h>

class Scene;

class SceneComponent {
	friend class Scene;
private:
	Scene* scene;
public:
	SceneComponent(Scene* scene);
	virtual ~SceneComponent();

	Scene* GetScene() const;

	virtual void OnPreUpdate();
	virtual void OnPostUpdate();

	virtual void OnPreRender();
	virtual void OnPostRender();

	virtual void DrawImGui();

	virtual int Order();

	virtual json Serialize();
	virtual void Deserialize(const json& data);
};
