#pragma once

#include <functional>
#include <string>

class SceneNode;

namespace Editor {
struct Context;

void OpenLoadSceneDialog(Context& context);
void OpenSaveSceneDialog(Context& context);

void OpenLoadPrefabDialog(Context& context);
void OpenSavePrefabDialog(Context& context);

void OpenSaveTextureDialog(Context& context,
                           std::function<void(std::string)> saveCallback);
} // namespace Editor
