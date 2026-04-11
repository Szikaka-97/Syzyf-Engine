#pragma once

class Scene;
class SceneNode;

namespace Editor {
bool Setup();
void Terminate();

void DrawGraphNode(SceneNode &node);
void DrawGraph(Scene &scene);
void DrawInspector();
void DrawFiles();

void MainLoop();
} // namespace Editor
