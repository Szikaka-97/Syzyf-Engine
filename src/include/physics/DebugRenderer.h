#pragma once

#include "BoundingBox.h"
#include "SceneComponent.h"
#include <Jolt/Jolt.h>
#include "physics/System.h"
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <glm/geometric.hpp>
#include <vector>
#include <glm/vec3.hpp>
#include <glad/glad.h> 


class ShaderProgram;

namespace Physics {
class IgnoreEditorLayerFilter : public JPH::BodyDrawFilter {
    public:
        virtual bool ShouldDraw(const JPH::Body& inBody) const override {
            return inBody.GetObjectLayer() != Layers::EDITOR;
        }
    };

class DebugRenderer : public JPH::DebugRendererSimple
{
public:
    IgnoreEditorLayerFilter filter;
public:
  DebugRenderer() = default;
  virtual ~DebugRenderer() = default;

  void Init(ShaderProgram* debugShader);

  virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
  virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
  virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) override;

  void DrawBoundingBox(const BoundingBox& box, JPH::ColorArg color, glm::mat4 transform); 
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
}
