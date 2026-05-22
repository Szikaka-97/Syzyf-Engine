#pragma once

#include <glm/glm.hpp>

#include <Serialized.h>

struct BoundingBox;

struct Plane {
	serialized glm::vec3 normal;
	serialized float distance;

	Plane() = default;
	Plane(const glm::vec3& normal, float distance);
};

struct Frustum {
	serialized Plane top;
	serialized Plane bottom;
	serialized Plane left;
	serialized Plane right;
	serialized Plane nearPlane;
	serialized Plane farPlane;

	Frustum() = default;
	Frustum(const Plane& top,
	        const Plane& bottom,
	        const Plane& left,
	        const Plane& right,
	        const Plane& nearPlane,
	        const Plane& farPlane
	);
};

Frustum ComputeFrustum(const glm::mat4& projectionMatrix);
bool TestPlane(const Plane& plane, const BoundingBox& bounds);
bool TestFrustum(const Frustum& frustum, const BoundingBox& bounds);
