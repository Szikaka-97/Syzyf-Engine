#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "ParticleSpawner.h"

class Font;
class ShaderProgram;
class Mesh;
class Material;
class MeshRenderer;

class Text3D : public GameObject, public ImGuiDrawable {
public:
    glm::vec4 color{1.0f};
    BillboardMode billboardMode = BillboardMode::Disabled;
private:
    std::string text = "";
    Font* font;

    std::unique_ptr<Mesh> mesh;
    MeshRenderer* renderer = nullptr;
    
    std::unique_ptr<ShaderProgram> shader;
    std::shared_ptr<Material> material;

public:
    Text3D(std::string text = "", Font* font = nullptr, std::shared_ptr<Material> material = nullptr);

    std::string GetText() const;
    Font* GetFont() const;

    void SetText(const std::string& newText);
    void SetFont(Font* newFont);

    void Render();
    void DrawImGui();

private:
    void RebuildMesh();
};
