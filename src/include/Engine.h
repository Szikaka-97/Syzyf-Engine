#pragma once

#include <concepts>

class SDL_Window;
class SDL_GLContextState;
class Scene;

template <typename T>
concept SceneCreationCallback = requires(T a, Scene* s) {
	{ a(s) } -> std::same_as<void>;
};

class Engine {
private:
	Engine() = delete;

	static SDL_Window* window;
	static SDL_GLContextState* glContext;
	static Scene* rootScene;

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
	static SDL_Window* GetWindow();
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