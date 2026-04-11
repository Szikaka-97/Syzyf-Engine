#pragma once

class Scene;
class SceneNode;

namespace Editor {
bool Setup();
void Terminate();

void DrawMainMenuBar(bool& shouldClose);
void DrawGraphNode(SceneNode& node);
void DrawGraph(Scene& scene);
void DrawInspector();
void DrawFiles();
void DrawSceneView(Scene& scene);

void MainLoop();
} // namespace Editor
