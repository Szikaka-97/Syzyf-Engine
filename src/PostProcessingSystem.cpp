#include <PostProcessingSystem.h>

PostProcessingSystem::PostProcessingSystem(Scene* scene):
GameObjectSystem<PostProcessor>(scene) { }