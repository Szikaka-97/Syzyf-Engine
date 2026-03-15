#include "Fog.h"
#include "Mesh.h"
#include "Shader.h"

Fog::Fog() {
  this->shader = ShaderProgram::Build()
    .WithVertexShader(
      GetScene()->Resources()->Get<VertexShader>("./res/shaders/fullscreen.vert")
    ).WithPixelShader(
      GetScene()->Resources()->Get<PixelShader>("./res/shaders/fog.frag")
    ).Link();
}

void Fog::OnPostProcess(const PostProcessParams* params) {
  glUseProgram(this->shader->GetHandle());

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

}
