#pragma once

#include "Scene.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <glm/glm.hpp>

class Mesh;

namespace Physics {

JPH::ShapeRefC SphereShape(float radius);
JPH::ShapeRefC BoxShape(glm::vec3 halfExtent);
JPH::ShapeRefC CapsuleShape(float halfHeight, float radius);
JPH::ShapeRefC PlaneShape(glm::vec3 normal);
JPH::ShapeRefC ConvexHullMeshShape(const Mesh* mesh);
JPH::ShapeRefC MeshShape(const Mesh* mesh, glm::vec3 scale = glm::vec3(1.0f));

JPH::ShapeRefC CreateCompoundShapeFromNode(SceneNode* rootNode, bool useConvex, JPH::EMotionType motionType, JPH::ObjectLayer layer);
}
