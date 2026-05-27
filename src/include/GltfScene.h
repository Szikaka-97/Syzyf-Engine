#pragma once

#include "Resources.h"
#include "Texture.h"
#include "animation/AnimationComponent.h"
#include "fastgltf/types.hpp"

#include <filesystem>

#include <fastgltf/core.hpp>

class Scene;
class SceneNode;
class Material;
class Mesh;

class GltfScene : public Resource {
public:
  static GltfScene* Load(const std::filesystem::path path);
  ~GltfScene();
  SceneNode* Instantiate(Scene* scene, SceneNode* parent = nullptr, std::string name = "");
private:
  std::unique_ptr<fastgltf::Asset> asset;
  std::vector<Mesh*> meshes;
  std::vector<Material*> materials;
  bool isSkinned = false;

  SceneNode* CreateNode(fastgltf::Node& gltfNode, Scene* scene, std::vector<SceneNode*>& sceneNodes, SceneNode* parent = nullptr);

  static std::vector<Material*> LoadMaterials(fastgltf::Asset& asset, bool isSkinned);
  static Mesh* LoadMesh(fastgltf::Mesh& mesh, fastgltf::Asset& asset, std::vector<Material*>& materials);
  static Texture2D* LoadImage(fastgltf::Asset& asset, fastgltf::Image& image, const TextureParams loadParams);
  static std::optional<AnimationComponent::Animation> LoadAnimation(std::vector<SceneNode*>& sceneNodes, fastgltf::Animation& gltfAnimation, fastgltf::Asset& asset);

  static TextureFilter GltfFilterToTextureFilter(fastgltf::Filter filter);
  static TextureWrap GltfWrapToTextureWrap(fastgltf::Wrap wrap);
  static void GltfSamplerToTextureParams(TextureParams& params, fastgltf::Sampler& sampler);
};
