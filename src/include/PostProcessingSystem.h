#pragma once

#include <GameObjectSystem.h>

#include <PostProcessor.h>

class PostProcessingSystem : public GameObjectSystem<PostProcessor> {
public:
	PostProcessingSystem(Scene* scene);
};