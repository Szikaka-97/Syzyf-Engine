#include "text/Text3D.h"

#include "Material.h"
#include "Mesh.h"
#include "Graphics.h"
#include "MeshRenderer.h"
#include "text/Font.h"

#include <cmath>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

Text3D::Text3D():
Text3D("") { }

Text3D::Text3D(std::string text, Font* font, std::shared_ptr<Material> material) : text(text), font(font), material(material), alignment(TextAlignment::Left) { }

void Text3D::Awake() {

    if (material == nullptr) {
        this->shader.reset(
            ShaderProgram::Build()
                .WithVertexShader("./res/shaders/text3d/text3d.vert")
                .WithPixelShader("./res/shaders/text3d/text3d.frag")
                .Link()
        );
        this->material = std::make_shared<Material>(this->shader.get());
    }
    
    this->material->SetValue("fontAtlas", font->atlasTexture);
    this->material->SetValue("textColor", glm::vec4(1.0f));
    this->material->SetValue("pxRange", static_cast<float>(font->distanceRange));

    this->renderer = this->GetNode()->AddObject<MeshRenderer>();
    this->RebuildMesh();
    this->renderer->SetMaterial(this->material.get());
    this->material->SetValue("textColor", this->color);
    this->material->SetValue("useMsdf", this->font->useMsdf);
}

std::string Text3D::GetText() const {
    return this->text;
}

Font* Text3D::GetFont() const {
    return this->font;
}

TextAlignment Text3D::GetAlignment() const {
    return this->alignment;
}

void Text3D::SetText(const std::string& newText) {
    if (this->text == newText) {
        return;
    }

    this->text = newText;
    this->RebuildMesh();
}

void Text3D::SetFont(Font* newFont) {
    if (this->font == newFont) {
        return;
    }

    this->font = newFont;
    this->RebuildMesh();
    this->material->SetValue("useMsdf", this->font->useMsdf);
}

void Text3D::SetAlignment(TextAlignment alignment) {
    if (this->alignment == alignment) {
        return;
    }

    this->alignment = alignment;
    this->RebuildMesh();
}

void Text3D::Render() {
    this->material->SetValue("textColor", this->color);
    this->material->SetValue("billboardMode", static_cast<unsigned int>(this->billboardMode));   
}

void Text3D::DrawImGui() {
    std::string previousText = this->text;
    if (ImGui::InputTextMultiline("Text", &this->text, (ImVec2(-1.0f, ImGui::GetTextLineHeight() * 5)))) {
        if (this->text != previousText) {
            this->RebuildMesh();
        }
    }

    ImGui::ColorEdit4("Text Color", &this->color[0]);

    const char* billboardModes[] = { "Disabled", "Enabled", "Z" };
    int currentBillboardMode = static_cast<int>(this->billboardMode);
    if (ImGui::Combo("Billboard Mode", &currentBillboardMode, billboardModes, IM_ARRAYSIZE(billboardModes))) {
        this->billboardMode = static_cast<BillboardMode>(currentBillboardMode);
    }

    const char* alignments[] = { "Left", "Middle", "Right" };
    int currentAlignment = static_cast<int>(this->alignment);
    if (ImGui::Combo("Text Alignment", &currentAlignment, alignments, IM_ARRAYSIZE(alignments))) {
        this->SetAlignment(static_cast<TextAlignment>(currentAlignment));
    }
}

void Text3D::RebuildMesh() {
    struct TextLine {
        float length;
        int numChars;
    };

    if (!this->font) return;
    if (this->mesh != nullptr) {
        this->mesh = nullptr;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;

    float xOffset = 0.0f;
    float yOffset = 0.0f;
    float scale = 1.0f / this->font->emSize;

    int charsInCurrentLine = 0;

    std::vector<TextLine> lineLenghts;
    float longestLine = 0;

    for (char c : this->text) {
        if (c == '\n') {
            lineLenghts.push_back({
                xOffset,
                charsInCurrentLine + 1
            });

            if (xOffset > longestLine) {
                longestLine = xOffset;
            }

            xOffset = 0.0f;
            charsInCurrentLine = 0;
        }
        else {
            charsInCurrentLine++;
            
            if (font->glyphs.find(c) == font->glyphs.end()) continue;

            const Glyph& glyph = font->glyphs[c];

            xOffset += glyph.advance * scale;
        }
    }

    lineLenghts.push_back({
        xOffset,
        charsInCurrentLine
    });

    if (xOffset > longestLine) {
        longestLine = xOffset;
    }

    int startingChar = 0;

    for (auto line : lineLenghts) {
        if (this->alignment == TextAlignment::Left) {
            xOffset = 0;
        }
        else if (this->alignment == TextAlignment::Middle) {
            xOffset = (longestLine - line.length) * 0.5f;
        }
        else {
            xOffset = longestLine - line.length;
        }

        yOffset -= font->lineHeight * scale;

        for (int i = 0; i < line.numChars; i++) {
            char c = this->text[startingChar + i];

            if (font->glyphs.find(c) == font->glyphs.end()) continue;

            const Glyph& glyph = font->glyphs[c];

            if (glyph.planeBounds.z > glyph.planeBounds.x) {
                float left = xOffset + glyph.planeBounds.x * scale;
                float bottom = yOffset + glyph.planeBounds.y * scale;
                float right = xOffset + glyph.planeBounds.z * scale;
                float top = yOffset + glyph.planeBounds.w * scale;

                float uvLeft = glyph.atlasBounds.x / font->atlasTexture->GetWidth();
                float uvBottom = glyph.atlasBounds.y / font->atlasTexture->GetHeight();
                float uvRight = glyph.atlasBounds.z / font->atlasTexture->GetWidth();
                float uvTop = glyph.atlasBounds.w / font->atlasTexture->GetHeight();

                int baseIndex = positions.size();

                positions.push_back({left, bottom, 0.0f});
                uvs.push_back({uvLeft, uvBottom});
                positions.push_back({right, bottom, 0.0f});
                uvs.push_back({uvRight, uvBottom});
                positions.push_back({right, top, 0.0f});
                uvs.push_back({uvRight, uvTop});
                positions.push_back({left, top, 0.0f});
                uvs.push_back({uvLeft, uvTop});

                indices.insert(indices.end(), {
                    (unsigned int)(baseIndex + 0), (unsigned int)(baseIndex + 1), (unsigned int)(baseIndex + 2),
                    (unsigned int)(baseIndex + 2), (unsigned int)(baseIndex + 3), (unsigned int)(baseIndex + 0)
                });
            }
            
            xOffset += glyph.advance * scale;
        }

        startingChar += line.numChars;
    }

    // if (positions.empty()) {
    //     this->mesh = nullptr;
    //     this->renderer->SetMesh(nullptr);
    //     return;
    // }

    glm::vec3 minCorner(INFINITY);
    glm::vec3 maxCorner(-INFINITY);

    for (const auto& position : positions) {
        minCorner = glm::min(minCorner, position);
        maxCorner = glm::max(maxCorner, position);
    }

    // Shifting the position so the origin is at the center
    float centerX = (minCorner.x + maxCorner.x) * 0.5f;
    float centerY = (minCorner.y + maxCorner.y) * 0.5f;
    for (auto& position : positions) {
        position.x -= centerX;
        position.y -= centerY;
    }
    minCorner.x -= centerX;
    maxCorner.x -= centerX;
    minCorner.y -= centerY;
    maxCorner.y -= centerY;

    this->mesh = std::make_unique<Mesh>();
    this->mesh->materialCount = 1;

    this->mesh->vertexCount = positions.size();
    this->mesh->vertexStride = VertexSpec::Mesh.VertexSize();
    this->mesh->vertexData = new float[mesh->vertexCount * mesh->vertexStride];

    memset((void*)mesh->vertexData, 0, mesh->vertexCount * mesh->vertexStride * sizeof(float));

    int positionOffset = 0;
    int normalOffset = positionOffset + VertexSpec::Mesh.GetLengthOf(VertexInputType::Position);
    int uvOffset = normalOffset + VertexSpec::Mesh.GetLengthOf(VertexInputType::Normal) + VertexSpec::Mesh.GetLengthOf(VertexInputType::Tangent);

    for (size_t i = 0; i < positions.size(); i++) {
        float* currentVertex = (float*)mesh->vertexData + (i * mesh->vertexStride);

        if (VertexSpec::Mesh.GetLengthOf(VertexInputType::Position)) {
            ((glm::vec4*)(currentVertex + positionOffset))->x = positions[i].x;
            ((glm::vec4*)(currentVertex + positionOffset))->y = positions[i].y;
            ((glm::vec4*)(currentVertex + positionOffset))->z = positions[i].z;
            ((glm::vec4*)(currentVertex + positionOffset))->w = 1.0f;
        }
        if (VertexSpec::Mesh.GetLengthOf(VertexInputType::Normal)) {
            ((glm::vec4*)(currentVertex + normalOffset))->x = 0.0f;
            ((glm::vec4*)(currentVertex + normalOffset))->y = 0.0f;
            ((glm::vec4*)(currentVertex + normalOffset))->z = 1.0f;
            ((glm::vec4*)(currentVertex + normalOffset))->w = 0.0f;
        }
        if (VertexSpec::Mesh.GetLengthOf(VertexInputType::UV1)) {
            ((glm::vec4*)(currentVertex + uvOffset))->x = uvs[i].x;
            ((glm::vec4*)(currentVertex + uvOffset))->y = uvs[i].y;
        }
    }

    Mesh::SubMesh submesh;
    submesh.bounds = BoundingBox(minCorner, maxCorner);
    submesh.type = Mesh::MeshType::Triangles;
    submesh.materialIndex = 0;
    submesh.faceCount = indices.size() / 3;
    submesh.indexData = new unsigned int[indices.size()];
    memcpy((void*)submesh.indexData, indices.data(), indices.size() * sizeof(unsigned int));

    this->mesh->subMeshes.push_back(submesh);
    this->mesh->vertexBuffer = mesh->UploadToGpu(VertexSpec::Mesh);
    this->mesh->CalculateBounds();

    this->renderer->SetMesh(this->mesh.get());
}
