#include "fog/Fog.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "imgui.h"

Fog::Fog(Fog::Type fogType, float minDistance, float maxDistance, glm::vec4 fogColor) : minDistance(minDistance), maxDistance(maxDistance), fogColor(fogColor) {
  this->shader = ShaderProgram::Build()
    .WithVertexShader(
      GetScene()->Resources()->Get<VertexShader>("./res/shaders/fullscreen.vert")
    ).WithPixelShader(
      GetScene()->Resources()->Get<PixelShader>("./res/shaders/fog/fog.frag")
    ).Link();

  this->material = new Material(this->shader);

  this->material->SetValue("fogColor", this->fogColor);
}

void Fog::OnPostProcess(const PostProcessParams* params) {
  this->material->SetValue("fogColor", this->fogColor);
  this->material->SetValue("minDistance", this->minDistance);
  this->material->SetValue("maxDistance", this->maxDistance);
  this->material->SetValue("density", this->density);
  this->material->SetValue("fogType", (unsigned int)this->fogType);
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
    const char* fogTypes[] = { "Linear", "Exponential", "Exponential Squared" };
    int currentType = (int) this->fogType;
    if (ImGui::Combo("FogType", &currentType, fogTypes, IM_ARRAYSIZE(fogTypes))) {
        this->fogType = (Type) currentType;
    }

    ImGui::ColorPicker4("Fog Color", &this->fogColor.x);

    if (this->fogType == Type::Linear) {
        ImGui::InputFloat("Min Distance", &this->minDistance);
        ImGui::InputFloat("Max Distance", &this->maxDistance);
    } else {
        ImGui::DragFloat("Density", &this->density, 0.001f, 0.0f, 1.0f);
    }
}
