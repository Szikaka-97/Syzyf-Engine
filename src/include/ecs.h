#pragma once

#include <limits>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <cstddef>

#include <spdlog/spdlog.h>

struct Entity {
  std::size_t id;
  std::size_t generation;
};

class ISparseSet {
public:
  virtual ~ISparseSet() = default;
};

template <typename T>
class SparseSet : public ISparseSet {
private:
  // Will act as the value representing a missing element in a dense array
  static constexpr std::size_t None = std::numeric_limits<std::size_t>::max();
public:
  std::vector<T> dense;
  std::vector<std::size_t> sparse;
  std::vector<Entity> reverse;

public:
  void Insert(Entity entity, T value) {
    this->dense.push_back(value);
    this->reverse.push_back(entity);

    if (entity.id >= this->sparse.size()) {
      this->sparse.resize(entity.id + 1, None);
    }

    std::size_t index = this->dense.size() - 1;
    this->sparse[entity.id] = index;
  }

  T* Get(Entity entity) {
    if (entity.id < this->sparse.size()) {
      std::size_t index = this->sparse[entity.id];
      if (index != None) {
        Entity reverseEntity = this->reverse[index];
        if (reverseEntity.generation == entity.generation) {
          return &this->dense[index];
        }
      }
    }
    spdlog::warn("ECS: Failed to retrieve an entity from a set");
    return nullptr;
  }

  void Remove(Entity entity) {
    if (this->reverse.empty() || entity.id >= this->sparse.size()) {
      spdlog::warn("ECS: Tried removing an entity which isn't present in the set");
      return;
    }

    std::size_t index = this->sparse[entity.id];
    if (index == None) {
      spdlog::warn("ECS: Tried removing an entity which isn't present in the set");
      return;
    }

    this->dense[index] = std::move(this->dense[dense.size() - 1]);
    this->dense.pop_back();
    this->sparse[this->reverse[reverse.size() - 1].id] = index;
    
    this->reverse[index] = this->reverse[reverse.size() - 1];
    this->reverse.pop_back();
    this->sparse[entity.id] = None; 
  }
};

class EntityBuilder;

class World {
private:
  struct Allocator {
    std::vector<std::size_t> free;
    std::vector<std::size_t> generations;

    Allocator();
    Entity Allocate();
  };

  Allocator allocator;
  std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> components;

public:
  Entity CreateEntity(); // Maybe remove, confusing
  EntityBuilder BuildEntity();

  template <typename T>
  void RegisterComponent() {
    std::type_index typeIndex = std::type_index(typeid(T));
  
    if (this->components.find(typeIndex) == this->components.end()) {
      this->components[typeIndex] = std::make_unique<SparseSet<T>>();
      return;
    }
    spdlog::warn("ECS: Failed to register a component, was it already registered?");
  }

  template <typename T>
  void AddComponent(Entity entity, T component) {
    std::type_index typeId = typeid(T);
    
    auto componentIter = this->components.find(typeId);
    if (componentIter != this->components.end()) {
      SparseSet<T>* set = static_cast<SparseSet<T>*>(componentIter->second.get());
      set->Insert(entity, component);
    }
  }

  template <typename T>
  SparseSet<T>* GetComponentVector() {
    std::type_index typeId = typeid(T);

    auto componentIter = this->components.find(typeId);
    if (componentIter != this->components.end()) {
      return static_cast<SparseSet<T>*>(componentIter->second.get());
    }
    spdlog::warn("ECS: Called GetComponentVector<T> where T is not a registered component");
    return nullptr;
  }

  template <typename T>
  T* GetComponent(Entity entity) {
    SparseSet<T>* storage = this->GetComponentVector<T>();
    if (storage == nullptr) {
      spdlog::warn("ECS: Called GetComponent<T> where T is not a registered component");
      return nullptr;
    }

    return storage->Get(entity);
  }
};

class EntityBuilder {
private:
  World& world;
  Entity entity;
public:
  EntityBuilder(World& world);

  template <typename T>
  EntityBuilder& With(T component) {
    this->world.AddComponent<T>(this->entity, component);
    return *this;
  }

  Entity Build();
};
