#include <Frustum.h>

#include "BoundingBox.h"

#include <glm/gtc/matrix_access.hpp>

Plane::Plane(const glm::vec3& normal, float distance):
normal(normal),
distance(distance) { }

Frustum::Frustum(const Plane& top,
        const Plane& bottom,
        const Plane& left,
        const Plane& right,
        const Plane& nearPlane,
        const Plane& farPlane
):
top(top),
bottom(bottom),
left(left),
right(right),
nearPlane(nearPlane),
farPlane(farPlane) { }

Frustum ComputeFrustum(const glm::mat4 &projectionMatrix) {
  Frustum result;

  const glm::vec4 planeLeftParams = glm::normalize(
      -(glm::row(projectionMatrix, 3) + glm::row(projectionMatrix, 0)));
  const glm::vec4 planeRightParams = glm::normalize(
      -(glm::row(projectionMatrix, 3) - glm::row(projectionMatrix, 0)));
  const glm::vec4 planeBottomParams = glm::normalize(
      -(glm::row(projectionMatrix, 3) + glm::row(projectionMatrix, 1)));
  const glm::vec4 planeTopParams = glm::normalize(
      -(glm::row(projectionMatrix, 3) - glm::row(projectionMatrix, 1)));
  const glm::vec4 planeNearParams = glm::normalize(
      -(glm::row(projectionMatrix, 3) + glm::row(projectionMatrix, 2)));
  const glm::vec4 planeFarParams = glm::normalize(
      -(glm::row(projectionMatrix, 3) - glm::row(projectionMatrix, 2)));

  result.left = Plane(glm::vec3(planeLeftParams), planeLeftParams.w);
  result.right = Plane(glm::vec3(planeRightParams), planeRightParams.w);
  result.bottom = Plane(glm::vec3(planeBottomParams), planeBottomParams.w);
  result.top = Plane(glm::vec3(planeTopParams), planeTopParams.w);
  result.nearPlane = Plane(glm::vec3(planeNearParams), planeNearParams.w);
  result.farPlane = Plane(glm::vec3(planeFarParams), planeFarParams.w);

  return result;
}

bool TestFrustum(const Frustum &frustum, const BoundingBox &bounds) {
  return (TestPlane(frustum.left, bounds) && TestPlane(frustum.right, bounds) &&
          TestPlane(frustum.bottom, bounds) && TestPlane(frustum.top, bounds) &&
          TestPlane(frustum.farPlane, bounds));
}

bool TestPlane(const Plane &plane, const BoundingBox &bounds) {
  const glm::vec3 n = plane.normal;
  const float d = plane.distance;

  const glm::vec3 c = bounds.center;
  const glm::vec3 h = bounds.GetExtents();

  const float e = h.x * glm::abs(glm::dot(n, glm::vec3(bounds.axisU))) +
                  h.y * glm::abs(glm::dot(n, glm::vec3(bounds.axisV))) +
                  h.z * glm::abs(glm::dot(n, glm::vec3(bounds.axisW)));

  const float s = glm::dot(c, n) + d;

  return s - e <= 0;
}
