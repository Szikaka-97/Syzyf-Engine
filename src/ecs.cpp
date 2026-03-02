#include "ecs.h"

World::Allocator::Allocator() {};

Entity World::Allocator::Allocate() {
  if (this->free.empty()) {
    this->generations.push_back(0);
      return Entity {
        this->generations.size() - 1,
        0,
      };
    };

    std::size_t id = this->free.back();
    this->free.pop_back();
    this->generations[id] += 1;
    return Entity {
    id,
    this->generations[id],
  };
}

Entity World::CreateEntity() {
  return this->allocator.Allocate();
}

EntityBuilder World::BuildEntity() {
  return EntityBuilder(*this);
}

EntityBuilder::EntityBuilder(World& world): world(world) {
  this->entity = world.CreateEntity();
}

Entity EntityBuilder::Build() {
  return this->entity;
}
