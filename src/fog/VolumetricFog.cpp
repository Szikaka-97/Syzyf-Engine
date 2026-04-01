#include "fog/VolumetricFog.h"

#include "Graphics.h"
#include "LightSystem.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "imgui.h"

VolumetricFog::VolumetricFog(
  float stepSize, float rayZFar, float scatteringDensity, float absorptionDensity, glm::vec3 scatteringColor, float k, float transmittanceThreshold
) : stepSize(stepSize), rayZFar(rayZFar), scatteringDensity(scatteringDensity), absorptionDensity(absorptionDensity), scatteringColor(scatteringColor), transmittanceThreshold(transmittanceThreshold) {
  this->shader = ShaderProgram::Build()
    .WithVertexShader(
      GetScene()->Resources()->Get<VertexShader>("./res/shaders/fullscreen.vert")
    ).WithPixelShader(
      GetScene()->Resources()->Get<PixelShader>("./res/shaders/fog/volumetric_fog.frag")
    ).Link();

  this->material = new Material(this->shader);

  this->material->SetValue("stepSize", this->stepSize);
  this->material->SetValue("rayZFar", this->rayZFar);
  this->material->SetValue("scatteringDensity", this->scatteringDensity);
  this->material->SetValue("absorptionDensity", this->absorptionDensity);
  this->material->SetValue("scatteringColor", this->scatteringColor);
  this->material->SetValue("k", this->k);
}

void VolumetricFog::OnPostProcess(const PostProcessParams* params) {
  this->material->SetValue("stepSize", this->stepSize);
  this->material->SetValue("rayZFar", this->rayZFar);
  this->material->SetValue("scatteringDensity", this->scatteringDensity);
  this->material->SetValue("absorptionDensity", this->absorptionDensity);
  this->material->SetValue("scatteringColor", this->scatteringColor);
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

  glActiveTexture(GL_TEXTURE2);
  GLuint shadowMapHandle = GetScene()->GetGraphics()->GetLightSystem()->GetShadowAtlasFramebuffer()->GetDepthTexture()->GetHandle();
  glBindTexture(GL_TEXTURE_2D, shadowMapHandle);
  location = glGetUniformLocation(this->shader->GetHandle(), "Builtin_ShadowMask");
  glUniform1i(location, 2);

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
  ImGui::SliderFloat("Step Size", &this->stepSize, 0.001f, 0.5f);
  ImGui::InputFloat("Ray Z Far", &this->rayZFar);
  ImGui::SliderFloat("Scattering Density", &this->scatteringDensity, 0.0f,
                     2.0f);
  ImGui::SliderFloat("Absorption Density", &this->absorptionDensity, 0.0f,
                     2.0f);
  ImGui::ColorPicker3("Scattering Color", &this->scatteringColor.x);
  // k = 0 isotropic, k > 0 forward scattering (like fog or dust), k < 0 backward scattering
  ImGui::SliderFloat("Anisotropy", &this->k, -0.99f, 0.99f);
}
