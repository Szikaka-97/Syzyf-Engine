#pragma once

#include <SceneComponent.h>

class ImGuiDrawable {
public:
	virtual void DrawImGui() = 0;
};

class DebugInspector : public SceneComponent {
public:
	DebugInspector(Scene* scene);

	virtual void DrawImGui();
	
	virtual int Order();
};