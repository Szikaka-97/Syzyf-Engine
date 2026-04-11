#pragma once

#include "Texture.h"
#include "animation/AnimationComponent.h"
#include "fastgltf/types.hpp"

#include <filesystem>

#include <fastgltf/core.hpp>

class Scene;
class SceneNode;
class Material;
class Mesh;

class GltfImporter {
public:
  static Scene* LoadScene(const std::filesystem::path path, std::string name = "");
private:
  static SceneNode* CreateNode(fastgltf::Node& gltfNode, std::vector<Material*>& materials, fastgltf::Asset& asset, Scene* scene, std::vector<SceneNode*>& sceneNodes, SceneNode* parent = nullptr);

  static std::vector<Material*> LoadMaterials(Scene* scene, fastgltf::Asset& asset, bool isSkinned);
  static std::optional<AnimationComponent::Animation> LoadAnimation(std::vector<SceneNode*>& sceneNodes, fastgltf::Animation& gltfAnimation, fastgltf::Asset& asset);
  static Mesh* LoadMesh(fastgltf::Mesh& mesh, fastgltf::Asset& asset, std::vector<Material*>& materials);
  static Texture2D* LoadImage(Scene* scene, fastgltf::Asset& asset, fastgltf::Image& image, const TextureParams loadParams);

  static TextureFilter GltfFilterToTextureFilter(fastgltf::Filter filter);
  static TextureWrap GltfWrapToTextureWrap(fastgltf::Wrap);
  static void GltfSamplerToTextureParams(TextureParams& params, fastgltf::Sampler& sampler);
};
