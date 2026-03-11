#pragma once

#include "BoundingBox.h"
#include "SceneComponent.h"
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <glm/geometric.hpp>
#include <vector>
#include <glm/vec3.hpp>
#include <glad/glad.h> 

class ShaderProgram;

class PhysicsDebugRenderer : public JPH::DebugRendererSimple, public SceneComponent
{
public:
  PhysicsDebugRenderer(Scene *scene);
  virtual ~PhysicsDebugRenderer() = default;

  virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
  virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
  virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) override;

  void DrawBoundingBox(const BoundingBox& box, JPH::ColorArg color); 
  void DrawFrustum(glm::mat4 viewProjection, JPH::ColorArg color);

  void Render();

private:
  struct DebugVertex {
    float x, y, z;
    float r, g, b;
  };

  ShaderProgram* shader;
  GLuint vao = 0;
  GLuint vbo = 0;
  std::vector<DebugVertex> lines;
};
