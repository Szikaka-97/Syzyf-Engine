#include "fog/VolumetricFog.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "imgui.h"

VolumetricFog::VolumetricFog(float near, float far, float stepSize, float rayZFar, float scatteringDensity, float absorptionDensity, glm::vec3 scatteringColor, float k) {
  this->shader = std::unique_ptr<ShaderProgram>(ShaderProgram::Build()
    .WithVertexShader(
      GetScene()->Resources()->Get<VertexShader>("./res/shaders/fullscreen.vert")
    ).WithPixelShader(
      GetScene()->Resources()->Get<PixelShader>("./res/shaders/fog/volumetric_fog.frag")
    ).Link());

  this->material = std::unique_ptr<Material>(new Material(this->shader.get()));

  this->material->SetValue("near", this->near);
  this->material->SetValue("far", this->far);
  this->material->SetValue("stepSize", this->stepSize);
  this->material->SetValue("rayZFar", this->rayZFar);
  this->material->SetValue("scatteringDensity", this->scatteringDensity);
  this->material->SetValue("absorptionDensity", this->absorptionDensity);
  this->material->SetValue("scatteringColor", this->scatteringDensity);
  this->material->SetValue("k", this->k);
}

void VolumetricFog::OnPostProcess(const PostProcessParams* params) {
  this->material->SetValue("near", this->near);
  this->material->SetValue("far", this->far);
  this->material->SetValue("stepSize", this->stepSize);
  this->material->SetValue("rayZFar", this->rayZFar);
  this->material->SetValue("scatteringDensity", this->scatteringDensity);
  this->material->SetValue("absorptionDensity", this->absorptionDensity);
  this->material->SetValue("scatteringColor", this->scatteringDensity);
  this->material->SetValue("k", this->k);

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

void VolumetricFog::DrawImGui() {
  ImGui::InputFloat("Near", &this->near);
  ImGui::InputFloat("Far", &this->far);
  ImGui::InputFloat("Step Size", &this->stepSize);
  ImGui::InputFloat("Ray Z Far", &this->rayZFar);
  ImGui::InputFloat("Scattering Density", &this->scatteringDensity);
  ImGui::InputFloat("Absorption Density", &this->absorptionDensity);
  ImGui::ColorPicker3("Scattering Color", &this->scatteringColor.x);
  ImGui::InputFloat("k", &this->k);
}
