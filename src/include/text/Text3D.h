#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "ParticleSpawner.h"
#include "ui/TextAlignment.h"

class Font;
class ShaderProgram;
class Mesh;
class Material;
class MeshRenderer;

class Text3D : public GameObject, public ImGuiDrawable {
public:
    serialized glm::vec4 color{1.0f};
    serialized BillboardMode billboardMode = BillboardMode::Disabled;
private:
    serialized std::string text = "";
    serialized Font* font;
    serialized TextAlignment alignment;

    std::unique_ptr<Mesh> mesh;
    serialized MeshRenderer* renderer = nullptr;
    
    std::unique_ptr<ShaderProgram> shader;
    std::shared_ptr<Material> material;

public:
    Text3D();
    Text3D(std::string text, Font* font = nullptr, std::shared_ptr<Material> material = nullptr);

    void Awake();

    std::string GetText() const;
    Font* GetFont() const;
    TextAlignment GetAlignment() const;

    void SetText(const std::string& newText);
    void SetFont(Font* newFont);
    void SetAlignment(TextAlignment alignment);

    void Render();
    void DrawImGui();

private:
    void RebuildMesh();
};
