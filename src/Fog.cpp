#include "Fog.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "imgui.h"

Fog::Fog(float near, float far, float minDistance, float maxDistance, glm::vec4 fogColor) : near(near), far(far), minDistance(minDistance), maxDistance(maxDistance), fogColor(fogColor) {
  this->shader = std::unique_ptr<ShaderProgram>(ShaderProgram::Build()
    .WithVertexShader(
      GetScene()->Resources()->Get<VertexShader>("./res/shaders/fullscreen.vert")
    ).WithPixelShader(
      GetScene()->Resources()->Get<PixelShader>("./res/shaders/fog.frag")
    ).Link());

  this->material = std::unique_ptr<Material>(new Material(this->shader.get()));

  this->material->SetValue("fogColor", this->fogColor);
}

void Fog::OnPostProcess(const PostProcessParams* params) {
  this->material->SetValue("fogColor", this->fogColor);
  this->material->SetValue("near", this->near);
  this->material->SetValue("far", this->far);
  this->material->SetValue("minDistance", this->minDistance);
  this->material->SetValue("maxDistance", this->maxDistance);
  this->material->SetValue("fogColor", this->fogColor);

  this->material->Bind();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, params->inputTexture->GetHandle());
  int location = glGetUniformLocation(this->shader->GetHandle(), "colorTex");
  glUniform1i(location, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, params->depthTexture->GetHandle());
  location = glGetUniformLocation(this->shader->GetHandle(), "depthTex");
  glUniform1i(location, 1);

  static Mesh* quadMesh = GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj");

  glDisable(GL_DEPTH_TEST);
  glBindVertexArray(quadMesh->SubMeshAt(0).GetVertexArrayHandle());

  glDrawElements(GL_TRIANGLES, quadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);

  glBindTexture(GL_TEXTURE_2D, 0);
  glEnable(GL_DEPTH_TEST);
  glBindVertexArray(0);
  glUseProgram(0);
}

void Fog::DrawImGui() {
  ImGui::InputFloat("Near", &this->near);
  ImGui::InputFloat("Far", &this->far);
  ImGui::InputFloat("Min Distance", &this->maxDistance);
  ImGui::InputFloat("Max Distance", &this->minDistance);
  ImGui::ColorPicker4("Fog Color", &this->fogColor.x);
}
