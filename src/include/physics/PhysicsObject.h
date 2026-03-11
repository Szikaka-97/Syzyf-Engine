#pragma once

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Mesh.h"
#include <Jolt/Physics/Body/BodyID.h>

#include <GameObject.h>
#include <glm/fwd.hpp>
#include <spdlog/spdlog.h>

using namespace JPH::literals;


class PhysicsObject : public GameObject {
    private:
        JPH::BodyID bodyId;
        JPH::BodyCreationSettings bodyCreationSettings;

        bool bodyCreated = false;
        bool addedToWorld = false;
    public:
    PhysicsObject(const JPH::BodyCreationSettings& settings);

    static JPH::BodyCreationSettings Sphere(float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer);
    static JPH::BodyCreationSettings Box(glm::vec3 halfExtent, const JPH::EMotionType type, const JPH::ObjectLayer layer);
    static JPH::BodyCreationSettings Capsule(float halfHeight, float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer);
    static JPH::BodyCreationSettings Plane(glm::vec3 normal, const JPH::EMotionType type, const JPH::ObjectLayer layer);
    static JPH::BodyCreationSettings ConvexHullMesh(const Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer);
    static JPH::BodyCreationSettings Mesh(const Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer);
  
    virtual ~PhysicsObject();

    JPH::BodyID getBodyId() const {
        return bodyId;
    }

    void SetShape(const JPH::ShapeRefC shape); 

    void Awake();
    void OnEnable();
    void OnDisable();

    private:
    PhysicsObject();
}; 
