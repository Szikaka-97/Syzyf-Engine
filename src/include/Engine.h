#pragma once

#include "ecs.h"
#include <concepts>

class GLFWwindow;
class Scene;

template <typename T>
concept SceneCreationCallback = requires(T a, Scene* s) {
	{ a(s) } -> std::same_as<void>;
};

class Engine {
private:
	Engine() = delete;

	static GLFWwindow* window;
	static Scene* rootScene;

  static World world;
  static void SetupWorld();

	static bool InitProgram();
	static bool InitImGui();
	static void Terminate();
	static void Update();
	static void Render();
	static void DrawImGui();
public:
	static bool Setup();
	template <SceneCreationCallback T>
	static bool Setup(T* sceneCreationCallback);

	static void MainLoop();
	static void Exit(int code = 0);

	static Scene* GetRoot();
	static GLFWwindow* GetWindow();
};

template <SceneCreationCallback T>
bool Engine::Setup(T* sceneCreationCallback) {
	bool result = Setup();

	if (!result) {
		return false;
	}

	sceneCreationCallback(rootScene);

	return true;
}
