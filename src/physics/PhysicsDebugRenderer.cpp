#include "physics/PhysicsDebugRenderer.h"
#include "Camera.h"
#include "Frustum.h"
#include "Resources.h"
#include "Shader.h"
#include "Scene.h"
#include <glm/gtc/type_ptr.hpp>

PhysicsDebugRenderer::PhysicsDebugRenderer(Scene *scene) : SceneComponent(scene) {
    shader = ShaderProgram::Build()
        .WithVertexShader(GetScene()->Resources()->Get<VertexShader>("./res/shaders/physics_debug/physics_debug.vert"))
        .WithPixelShader(GetScene()->Resources()->Get<PixelShader>("./res/shaders/physics_debug/physics_debug.frag"))
        .Link();
    
    if (shader) {
        shader->SetCastsShadows(false);
        shader->SetIgnoresDepthPrepass(true);
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(6);

    glBindVertexArray(0);
}

void PhysicsDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    JPH::Vec4 color = inColor.ToVec4();

    lines.push_back({
        (float)inFrom.GetX(), (float)inFrom.GetY(), (float)inFrom.GetZ(),
        color.GetX(), color.GetY(), color.GetZ()
    });

    lines.push_back({
        (float)inTo.GetX(), (float)inTo.GetY(), (float)inTo.GetZ(),
        color.GetX(), color.GetY(), color.GetZ()
    });
}

void PhysicsDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) {}

void PhysicsDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) {}

void PhysicsDebugRenderer::DrawBoundingBox(const BoundingBox& box, JPH::ColorArg color) {
  JPH::Vec3 halfExtents(box.axisU.w, box.axisV.w, box.axisW.w);
  JPH::AABox boxLocal(-halfExtents, halfExtents);

  glm::vec3 u = glm::vec3(box.axisU);
  glm::vec3 v = glm::vec3(box.axisV);
  glm::vec3 w = glm::vec3(box.axisW);
  glm::vec3 p = box.center;

  JPH::Mat44 transform(
    JPH::Vec4(u.x, u.y, u.z, 0),
    JPH::Vec4(v.x, v.y, v.z, 0),
    JPH::Vec4(w.x, w.y, w.z, 0),
    JPH::Vec4(p.x, p.y, p.z, 1)
  );

  DrawWireBox(transform, boxLocal, color);
}

void PhysicsDebugRenderer::DrawFrustum(glm::mat4 viewProjection, JPH::ColorArg color) {
  glm::mat4 inverseViewProjection = glm::inverse(viewProjection);

  glm::vec4 ndcCube[8] = {
    {-1.0f, -1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, -1.0f, 1.0f},
    {1.0f, 1.0f, -1.0f, 1.0f},
    {-1.0f, 1.0f, -1.0f, 1.0f},
    {-1.0f, -1.0f, 1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, 1.0f, 1.0f},
  };

  glm::vec3 frustumCorners[8];
  for (int i = 0; i < 8; ++i) {
    glm::vec4 corner = inverseViewProjection * ndcCube[i];
    frustumCorners[i] = glm::vec3(corner) / corner.w;
  }

  DrawLine(
    JPH::RVec3(frustumCorners[0].x, frustumCorners[0].y, frustumCorners[0].z),
    JPH::RVec3(frustumCorners[1].x, frustumCorners[1].y, frustumCorners[1].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[1].x, frustumCorners[1].y, frustumCorners[1].z),
    JPH::RVec3(frustumCorners[2].x, frustumCorners[2].y, frustumCorners[2].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[2].x, frustumCorners[2].y, frustumCorners[2].z),
    JPH::RVec3(frustumCorners[3].x, frustumCorners[3].y, frustumCorners[3].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[3].x, frustumCorners[3].y, frustumCorners[3].z),
    JPH::RVec3(frustumCorners[0].x, frustumCorners[0].y, frustumCorners[0].z),
    color
  );

  DrawLine(
    JPH::RVec3(frustumCorners[4].x, frustumCorners[4].y, frustumCorners[4].z),
    JPH::RVec3(frustumCorners[5].x, frustumCorners[5].y, frustumCorners[5].z),
    color);
  DrawLine(
    JPH::RVec3(frustumCorners[5].x, frustumCorners[5].y, frustumCorners[5].z),
    JPH::RVec3(frustumCorners[6].x, frustumCorners[6].y, frustumCorners[6].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[6].x, frustumCorners[6].y, frustumCorners[6].z),
    JPH::RVec3(frustumCorners[7].x, frustumCorners[7].y, frustumCorners[7].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[7].x, frustumCorners[7].y, frustumCorners[7].z),
    JPH::RVec3(frustumCorners[4].x, frustumCorners[4].y, frustumCorners[4].z),
    color
  );

  DrawLine(
    JPH::RVec3(frustumCorners[0].x, frustumCorners[0].y, frustumCorners[0].z),
    JPH::RVec3(frustumCorners[4].x, frustumCorners[4].y, frustumCorners[4].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[1].x, frustumCorners[1].y, frustumCorners[1].z),
    JPH::RVec3(frustumCorners[5].x, frustumCorners[5].y, frustumCorners[5].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[2].x, frustumCorners[2].y, frustumCorners[2].z),
    JPH::RVec3(frustumCorners[6].x, frustumCorners[6].y, frustumCorners[6].z),
    color
  );
  DrawLine(
    JPH::RVec3(frustumCorners[3].x, frustumCorners[3].y, frustumCorners[3].z),
    JPH::RVec3(frustumCorners[7].x, frustumCorners[7].y, frustumCorners[7].z),
    color
  );
}

void PhysicsDebugRenderer::Render() {
    if (lines.empty() || !shader) return;

    GLuint shaderHandle = shader->GetHandle();
    glUseProgram(shaderHandle);

    std::vector<Camera*> cameraObjects = GetScene()->FindObjectsOfType<Camera>();

    if (cameraObjects.empty()) return;

    glm::mat4 vp = cameraObjects.front()->ViewProjectionMatrix();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(DebugVertex), lines.data(), GL_STREAM_DRAW);

    glDrawArrays(GL_LINES, 0, (GLsizei)lines.size());

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    lines.clear();
}
