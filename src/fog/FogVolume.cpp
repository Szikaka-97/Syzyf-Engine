#include "fog/FogVolume.h"
#include <Graphics.h>
#include <Material.h>
#include <Mesh.h>
#include <Resources.h>
#include <Scene.h>
#include <Shader.h>
#include <imgui.h>

FogVolume::FogVolume() {
  this->mesh = GetScene()->Resources()->Get<Mesh>("./res/models/not_cube.obj");

  ShaderProgram *prog =
      ShaderProgram::Build()
          .WithVertexShader(GetScene()->Resources()->Get<VertexShader>(
              "./res/shaders/fog/fog_volume.vert"))
          .WithPixelShader(GetScene()->Resources()->Get<PixelShader>(
              "./res/shaders/fog/fog_volume.frag"))
          .Link();

  prog->SetVolumetric(true);

  this->material = new Material(prog);
}

void FogVolume::Render() {
  this->material->SetValue("stepSize", this->stepSize);
  this->material->SetValue("scatteringDensity", this->scatteringDensity);
  this->material->SetValue("absorptionDensity", this->absorptionDensity);
  this->material->SetValue("scatteringColor", this->scatteringColor);
  this->material->SetValue("k", this->k);
  this->material->SetValue("transmittanceThreshold",
                           this->transmittanceThreshold);

  GetScene()->GetGraphics()->DrawMesh(this->mesh, 0, this->material,
                                      this->GlobalTransform());
}

void FogVolume::DrawImGui() {
  ImGui::Text("Fog Volume Settings");
  ImGui::SliderFloat("Step Size", &this->stepSize, 0.001f, 0.5f);
  ImGui::SliderFloat("Scattering Density", &this->scatteringDensity, 0.0f,
                     2.0f);
  ImGui::SliderFloat("Absorption Density", &this->absorptionDensity, 0.0f,
                     2.0f);
  ImGui::ColorEdit3("Scattering Color", &this->scatteringColor.x);
  ImGui::SliderFloat("Anisotropy", &this->k, -0.99f, 0.99f);
  ImGui::InputFloat("Transmittance Threshold", &this->transmittanceThreshold);
}
