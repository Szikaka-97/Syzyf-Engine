#pragma once

class SceneNode;

namespace Editor {
struct Context;

void OpenLoadSceneDialog(Context& context);
void OpenSaveSceneDialog(Context& context);

void OpenLoadPrefabDialog(Context& context);
void OpenSavePrefabDialog(Context& context);
} // namespace Editor
