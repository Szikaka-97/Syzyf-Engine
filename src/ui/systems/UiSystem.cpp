#include "ui/systems/UiSystem.h"
#include "SceneComponent.h"
#include "Scene.h"
#include "ui/systems/UiInteractableSystem.h"
#include "ui/systems/UiLayoutSystem.h"
#include "ui/systems/UiRenderSystem.h"
#include "ui/systems/UiTextRenderSystem.h"

UiSystem::UiSystem(Scene* scene) : SceneComponent(scene) {
    scene->AddComponent<UiLayoutSystem>();
    scene->AddComponent<UiRenderSystem>();
    scene->AddComponent<UiTextRenderSystem>();
    scene->AddComponent<UiInteractableSystem>();
}
