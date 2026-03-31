#include "GltfImporter.h"

#include "Camera.h"
#include "Material.h"
#include "Mesh.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "Texture.h"
#include "VertexSpec.h"
#include "animation/AnimationComponent.h"
#include "animation/SkeletonComponent.h"

#include <fastgltf/math.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <glm/trigonometric.hpp>
#include <spdlog/spdlog.h>

Scene* GltfImporter::LoadScene(const fs::path path, std::string name) {
  Scene* scene = new Scene(); 

  if (!std::filesystem::exists(path)) {
    spdlog::warn("GltfImporter: File not found: {}", path.string());
    return nullptr;
  }

  fastgltf::Parser parser(fastgltf::Extensions::KHR_materials_emissive_strength);

  constexpr auto gltfOptions =
    fastgltf::Options::DontRequireValidAssetMember |
    fastgltf::Options::AllowDouble |
    fastgltf::Options::LoadExternalImages |
    fastgltf::Options::LoadExternalBuffers |
    fastgltf::Options::GenerateMeshIndices;

  auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
  if (!bool(gltfFile)) {
    spdlog::warn("Failed to open glTF file: {}\n\tPath:{}", fastgltf::getErrorMessage(gltfFile.error()), path.string());
    return nullptr;
  }

  auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
  if (asset.error() != fastgltf::Error::None) {
    spdlog::warn("Failed to load glTF: {}\n\tPath: {}", fastgltf::getErrorMessage(asset.error()), path.string());
    return nullptr;
  }
  
  if (!asset->defaultScene.has_value()) {
    spdlog::warn("glTF file is missing a default scene: {}", path.string());
    return nullptr;
  }

  bool isSkinned = !asset->skins.empty() || !asset->animations.empty();
  std::vector<Material*> materials = LoadMaterials(scene, asset.get(), isSkinned);
  
  SceneNode* root = scene->CreateNode(name);
  auto& nodeIndices = asset->scenes[asset->defaultScene.value()].nodeIndices;

  // Saving scene nodes to be able to add them as targets when adding animations
  std::vector<SceneNode*> sceneNodes;
  sceneNodes.resize(asset->nodes.size());

  for (auto& index : nodeIndices) {
    auto& node = asset->nodes[index];
    SceneNode* sceneNode = CreateNode(node, materials, asset.get(), scene, sceneNodes, root);
    sceneNodes[index] = sceneNode;
  }

  for (std::size_t i = 0; i < asset->nodes.size(); ++i) {
    auto& gltfNode = asset->nodes[i];

    if (gltfNode.skinIndex.has_value() && sceneNodes[i] != nullptr) {
      auto& gltfSkin = asset->skins[gltfNode.skinIndex.value()];

      auto* skeleton = sceneNodes[i]->AddObject<SkeletonComponent>();

      skeleton->joints.reserve(gltfSkin.joints.size());
      for (std::size_t jointIndex : gltfSkin.joints) {
        skeleton->joints.push_back(sceneNodes[jointIndex]);
      }

      skeleton->inverseBindMatrices.resize(gltfSkin.joints.size(), glm::mat4(1.0f));
      if (gltfSkin.inverseBindMatrices.has_value()) {
        auto& ibmAccessor = asset->accessors[gltfSkin.inverseBindMatrices.value()];

        fastgltf::iterateAccessorWithIndex<fastgltf::math::fmat4x4>(asset.get(), ibmAccessor, [&](fastgltf::math::fmat4x4 matrix, std::size_t idx) {
            skeleton->inverseBindMatrices[idx] = glm::make_mat4(matrix.data());
        });
      }

      if (gltfSkin.skeleton.has_value()) {
        skeleton->skeletonRoot = sceneNodes[gltfSkin.skeleton.value()];
      }
    }
  }

  // Animation
  if (!asset->animations.empty()) {
  root->AddObject<AnimationComponent>();
  auto* animationComponent = root->GetObject<AnimationComponent>();
  animationComponent->animations.reserve(asset->animations.size());
    for (auto& gltfAnimation : asset->animations) {
      auto animation = LoadAnimation(sceneNodes, gltfAnimation, asset.get());
      if (animation.has_value()) {
        animationComponent->animations.push_back(std::move(animation.value()));
      } else {
        continue;
      }
    }
  }

  return scene; 
}

std::optional<AnimationComponent::Animation> GltfImporter::LoadAnimation(
  std::vector<SceneNode*>& sceneNodes,
  fastgltf::Animation& gltfAnimation,
  fastgltf::Asset& asset
  ) {
  AnimationComponent::Animation animation;
  animation.data.name = gltfAnimation.name;
  animation.data.duration = 0.0f;
  animation.currentKeyframes.resize(gltfAnimation.channels.size());
  
  for (std::size_t i = 0; i < gltfAnimation.channels.size(); ++i) {
    fastgltf::AnimationChannel& channel = gltfAnimation.channels[i];
    
    if (!channel.nodeIndex.has_value()) {
      spdlog::warn("GltfImporter: Tried loading an animation track without missing target node");
      return std::nullopt;
    }

    AnimationComponent::Track track;
    track.target = sceneNodes[channel.nodeIndex.value()];

    switch (channel.path) {
      case fastgltf::AnimationPath::Rotation: track.property = AnimationComponent::Property::ROTATION; break;
      case fastgltf::AnimationPath::Scale: track.property = AnimationComponent::Property::SCALE; break;
      case fastgltf::AnimationPath::Translation: track.property = AnimationComponent::Property::POSITION; break;
      case fastgltf::AnimationPath::Weights: track.property = AnimationComponent::Property::WEIGHTS; break;
    }

    fastgltf::AnimationSampler& sampler = gltfAnimation.samplers[channel.samplerIndex];

    switch (sampler.interpolation) {
      case fastgltf::AnimationInterpolation::CubicSpline: track.interpolation = AnimationComponent::Interpolation::CUBICSPLINE; break;
      case fastgltf::AnimationInterpolation::Linear: track.interpolation = AnimationComponent::Interpolation::LINEAR; break;
      case fastgltf::AnimationInterpolation::Step: track.interpolation = AnimationComponent::Interpolation::STEP; break;
    }
    
    auto& inputAccessor = asset.accessors[sampler.inputAccessor];
    if (!inputAccessor.bufferViewIndex.has_value()) {
      spdlog::warn("GltfImporter: Tried loading an animation track with missing input data");
      continue;
    }

    const float maxInput = static_cast<float>(inputAccessor.max->get<double>(0));
    if (animation.data.duration < maxInput) {
      animation.data.duration = maxInput; 
    }

    track.inputs.resize(inputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(asset, inputAccessor, [&](float input, std::size_t index) {
      track.inputs[index] = input;
    });

    auto& outputAccessor = asset.accessors[sampler.outputAccessor];
    if (!outputAccessor.bufferViewIndex.has_value()) {
      spdlog::warn("GltfImporter: Tried loading an animation track with missing output data");
      continue;
    }

    if (track.property == AnimationComponent::Property::POSITION
        || track.property == AnimationComponent::Property::SCALE) {
      track.outputs.resize(outputAccessor.count * 3);
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, outputAccessor, [&](fastgltf::math::fvec3 output, std::size_t index) {
        track.outputs[index * 3] = output.x();
        track.outputs[index * 3 + 1] = output.y();
        track.outputs[index * 3 + 2] = output.z();
      });
    } else if (track.property == AnimationComponent::Property::ROTATION) {
      track.outputs.resize(outputAccessor.count * 4);
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, outputAccessor, [&](fastgltf::math::fvec4 output, std::size_t index) {
        track.outputs[index * 4] = output.x();
        track.outputs[index * 4 + 1] = output.y();
        track.outputs[index * 4 + 2] = output.z();
        track.outputs[index * 4 + 3] = output.w();
      });
    } else {
      track.outputs.resize(outputAccessor.count);
      fastgltf::iterateAccessorWithIndex<float>(asset, outputAccessor, [&](float output, std::size_t index) {
        track.outputs[index] = output;
      });
    }

    // maybe resize at the start idk dnsfdafdfnkjdsabfksavsa
    animation.data.tracks.push_back(track);
  }

  return animation;
}

SceneNode* GltfImporter::CreateNode(
    fastgltf::Node& gltfNode,
    std::vector<Material*>& materials,
    fastgltf::Asset& asset,
    Scene* scene,
    std::vector<SceneNode*>& sceneNodes,
    SceneNode* parent
  ) {
  SceneNode* node = scene->CreateNode(parent, gltfNode.name.c_str());
  
  // LocalTransform
  std::visit(fastgltf::visitor {
    [&](fastgltf::TRS& trs) {
      node->LocalTransform().Position() = glm::make_vec3(trs.translation.data());
      node->LocalTransform().Rotation() = glm::make_quat(trs.rotation.data());
      node->LocalTransform().Scale() = glm::make_vec3(trs.scale.data());
    },
    [&](fastgltf::math::fmat4x4& matrix) {
      node->LocalTransform() = glm::make_mat4(matrix.data());
    }
  }, gltfNode.transform);

  // Mesh
  if (gltfNode.meshIndex.has_value()) {
    auto& gltfMesh = asset.meshes[gltfNode.meshIndex.value()];
    Mesh* mesh = LoadMesh(gltfMesh, asset, materials);
    node->AddObject<MeshRenderer>(mesh, materials);
  }

  // Camera
  //
  // gltf cameras look down the negative -z
  //  so it's looking in the opposite direction i guess
  //  couldn't rotate it here
  if (gltfNode.cameraIndex.has_value()) {
    auto& gltfCamera = asset.cameras[gltfNode.cameraIndex.value()].camera;
    std::visit(fastgltf::visitor {
      [&](fastgltf::Camera::Orthographic& camera) {
        node->AddObject<Camera>(Camera::Orthographic(
          -camera.xmag,
          camera.xmag,
          camera.ymag,
          -camera.ymag,
          camera.znear,
          camera.zfar
        ));
      },
      [&](fastgltf::Camera::Perspective& camera) {
        node->AddObject<Camera>(Camera::Perspective(
          glm::degrees(camera.yfov),
          camera.aspectRatio.value_or(1.7f), // these should be somewhere else
          camera.znear,      // or it should fail to add the camera if these are missing
          camera.zfar.value_or(300.0f)
        ));
      },
    }, gltfCamera);
  }
  
  for (auto& childIndex : gltfNode.children) {
    auto& childNode = asset.nodes[childIndex];
    SceneNode* sceneNode = CreateNode(childNode, materials, asset, scene, sceneNodes, node);
    sceneNodes[childIndex] = sceneNode;
  }

  return node;
}

Mesh* GltfImporter::LoadMesh(fastgltf::Mesh& gltfMesh, fastgltf::Asset& asset, std::vector<Material*>& materials) {
  Mesh* mesh = new Mesh;
  mesh->subMeshes.resize(gltfMesh.primitives.size());
  mesh->materials = materials;
  mesh->materialCount = materials.size(); 

  std::size_t vertexCount = 0;
  for (std::size_t primitiveIndex = 0; primitiveIndex < mesh->subMeshes.size(); ++primitiveIndex) {
    auto& primitive = mesh->subMeshes[primitiveIndex];
    
    switch (gltfMesh.primitives[primitiveIndex].type) {
      case fastgltf::PrimitiveType::Points: primitive.type = Mesh::MeshType::Points; break;
      case fastgltf::PrimitiveType::Lines: primitive.type = Mesh::MeshType::Lines; break;
      case fastgltf::PrimitiveType::Triangles: primitive.type = Mesh::MeshType::Triangles; break;
      default: spdlog::warn("GltfImporter: Tried loading a mesh with an unsupported primitive type: {}", gltfMesh.name); continue;
    }

    auto* positionIt = gltfMesh.primitives[primitiveIndex].findAttribute("POSITION");
    assert(positionIt != gltfMesh.primitives[primitiveIndex].attributes.end());
    assert(gltfMesh.primitives[primitiveIndex].indicesAccessor.has_value());

    auto& positionAccessor = asset.accessors[positionIt->accessorIndex];
    if (!positionAccessor.bufferViewIndex.has_value()) {
      spdlog::warn("GltfImporter: positionAccessor.bufferViewIndex has no value");
      continue;
    }

    auto& indicesAccessor = asset.accessors[gltfMesh.primitives[primitiveIndex].indicesAccessor.value()];
    if (!indicesAccessor.bufferViewIndex.has_value()) {
      spdlog::warn("GltfImporter: indicesAccessor.bufferViewIndex has no value");
      continue;
    }
    
    vertexCount += positionAccessor.count;
    primitive.faceCount = indicesAccessor.count / (unsigned int)primitive.type;
  }

  bool isSkinned = false;
  if (!gltfMesh.primitives.empty()) {
    isSkinned = gltfMesh.primitives[0].findAttribute("JOINTS_0") != gltfMesh.primitives[0].attributes.end();
  }

	VertexSpec meshSpec = isSkinned ? VertexSpec::MeshSkinned : VertexSpec::Mesh;
  mesh->vertexData = new float[vertexCount * meshSpec.VertexSize() + 3];
  memset(mesh->vertexData, 0, vertexCount * meshSpec.VertexSize() * sizeof(float));
  mesh->vertexCount = vertexCount;
  mesh->vertexStride = meshSpec.VertexSize();

  int vertexPointer = 0;
  int normalOffset = meshSpec.GetLengthOf(VertexInputType::Position);
  int binormalOffset = normalOffset + meshSpec.GetLengthOf(VertexInputType::Normal);
  int tangentOffset = binormalOffset + meshSpec.GetLengthOf(VertexInputType::Binormal);
  int uv1Offset = tangentOffset + meshSpec.GetLengthOf(VertexInputType::Tangent);
  int uv2Offset = uv1Offset + meshSpec.GetLengthOf(VertexInputType::UV1);
  int colorOffset = uv2Offset + meshSpec.GetLengthOf(VertexInputType::UV2);
  int jointsOffset = colorOffset + meshSpec.GetLengthOf(VertexInputType::Color);
  int weightsOffset = jointsOffset + meshSpec.GetLengthOf(VertexInputType::Joints);
  
  for (auto it = gltfMesh.primitives.begin(); it != gltfMesh.primitives.end(); ++it) {
    auto* positionIt = it->findAttribute("POSITION");
    assert(positionIt != it->attributes.end()); // ??
    assert(it->indicesAccessor.has_value()); // Generating indices so should always be true

    std::size_t baseColorTexcoordIndex = 0;

    auto index = std::distance(gltfMesh.primitives.begin(), it);
    auto& primitive = mesh->subMeshes[index];

    if (it->materialIndex.has_value()) {
#warning GltfImporter: Add a defualt material and offset this by one
      primitive.materialIndex = it->materialIndex.value();
    } else {
      primitive.materialIndex = 0;
    }

    primitive.indexData = new unsigned int[primitive.faceCount * (unsigned int) primitive.type];

    auto& positionAccessor = asset.accessors[positionIt->accessorIndex];
    
    if (positionAccessor.min.has_value() && positionAccessor.max.has_value()
        && positionAccessor.min.value().size() == 3 && positionAccessor.max.value().size() == 3
      ) {
      primitive.bounds = BoundingBox(
        glm::vec3(
          static_cast<float>(positionAccessor.min->get<double>(0)),
          static_cast<float>(positionAccessor.min->get<double>(1)),
          static_cast<float>(positionAccessor.min->get<double>(2))
        ),
        glm::vec3(
          static_cast<float>(positionAccessor.max->get<double>(0)),
          static_cast<float>(positionAccessor.max->get<double>(1)),
          static_cast<float>(positionAccessor.max->get<double>(2))
        )
      );
    }

    if (positionIt != it->attributes.end()) {
      auto& positionAccessor = asset.accessors[positionIt->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccessor, [&](fastgltf::math::fvec3 position, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride);
        target[0] = position.x();
        target[1] = position.y();
        target[2] = position.z();
      });
    }

    auto* normalIt = it->findAttribute("NORMAL");
    if (normalIt != it->attributes.end()) {
      auto& normalAccessor = asset.accessors[normalIt->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, normalAccessor, [&](fastgltf::math::fvec3 normal, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride) + normalOffset;
        target[0] = normal.x();
        target[1] = normal.y();
        target[2] = normal.z();
      });
    }

    auto* tangentIt = it->findAttribute("TANGENT");
    if (tangentIt != it->attributes.end()) {
      auto& tangentAccessor = asset.accessors[tangentIt->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, tangentAccessor, [&](fastgltf::math::fvec4 tangent, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride) + tangentOffset;
        target[0] = tangent.x();
        target[1] = tangent.y();
        target[2] = tangent.z();
        target[3] = tangent.w();
      });
    }

    auto* texcoord0It = it->findAttribute("TEXCOORD_0");
    if (texcoord0It != it->attributes.end()) {
      auto& texcoord0Accessor = asset.accessors[texcoord0It->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, texcoord0Accessor, [&](fastgltf::math::fvec2 uv, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride) + uv1Offset;
        target[0] = uv.x();
        target[1] = uv.y();
      });
    }

    auto* texcoord1It = it->findAttribute("TEXCOORD_1");
    if (texcoord1It != it->attributes.end()) {
      auto& texcoord1Accessor = asset.accessors[texcoord1It->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, texcoord1Accessor, [&](fastgltf::math::fvec2 uv, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride) + uv2Offset;
        target[0] = uv.x();
        target[1] = uv.y();
      });
    }

    auto* colorIt = it->findAttribute("COLOR_0");
    if (colorIt != it->attributes.end()) {
      auto& colorAccessor = asset.accessors[colorIt->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, colorAccessor, [&](fastgltf::math::fvec4 color, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride) + colorOffset;
        target[0] = color.x();
        target[1] = color.y();
        target[2] = color.z();
        target[3] = color.w();
      });
    }

    auto* jointsIt = it->findAttribute("JOINTS_0");
    if (jointsIt != it->attributes.end()) {
      auto& jointsAccessor = asset.accessors[jointsIt->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, jointsAccessor, [&](fastgltf::math::fvec4 joints, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride) + jointsOffset;
        target[0] = joints.x();
        target[1] = joints.y();
        target[2] = joints.z();
        target[3] = joints.w();
      });
    }

    auto* weightsIt = it->findAttribute("WEIGHTS_0");
    if (weightsIt != it->attributes.end()) {
      auto& weightsAccessor = asset.accessors[weightsIt->accessorIndex];
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, weightsAccessor, [&](fastgltf::math::fvec4 weights, std::size_t index) {
        float* target = mesh->vertexData + ((vertexPointer + index) * mesh->vertexStride) + weightsOffset;
        target[0] = weights.x();
        target[1] = weights.y();
        target[2] = weights.z();
        target[3] = weights.w();
      });
    }

    auto& indicesAccessor = asset.accessors[it->indicesAccessor.value()];
    fastgltf::iterateAccessorWithIndex<std::uint32_t>(asset, indicesAccessor, [&](std::uint32_t ind, std::size_t index) {
      primitive.indexData[index] = ind + vertexPointer;
    });

    vertexPointer += positionAccessor.count;
  }

  mesh->vertexBuffer = mesh->UploadToGpu(meshSpec);

  return mesh;
}

// The same image will be loaded multiple times if it's bundled, add a local cache or load them all at once earlier 
//  This ignores the sampler right now
Texture2D* GltfImporter::LoadImage(Scene* scene, fastgltf::Asset& asset, fastgltf::Image& image, const TextureParams loadParams) {
  Texture2D* result = nullptr;

  std::visit(fastgltf::visitor {
    [](auto& arg) {},
    [&](fastgltf::sources::URI& filePath) {
      const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
      result = scene->Resources()->Get<Texture2D>(path, loadParams);
    },
    [&](fastgltf::sources::Array& vector) {
      const unsigned char* data = reinterpret_cast<const unsigned char*>(vector.bytes.data());
      int length = static_cast<int>(vector.bytes.size());
      // SKIPS RESOURCE MANAGER -> no caching
      result = Texture2D::Load(data, length, loadParams);
    },
    [&](fastgltf::sources::BufferView& view) {
      auto& bufferView = asset.bufferViews[view.bufferViewIndex];
      auto& buffer = asset.buffers[bufferView.bufferIndex];

      std::visit(fastgltf::visitor {
        [](auto& arg) {},
        [&](fastgltf::sources::Array& vector) {
          const unsigned char* data = reinterpret_cast<const unsigned char*>(vector.bytes.data() + bufferView.byteOffset);
int length = static_cast<int>(bufferView.byteLength);
          // SKIPS RESOURCE MANAGER -> no caching
result = Texture2D::Load(data, length, loadParams);
        }
      }, buffer.data);
    },
  }, image.data);

  return result;
}

TextureFilter GltfImporter::GltfFilterToTextureFilter(fastgltf::Filter filter) {
  switch (filter) {
    case fastgltf::Filter::Linear:
      return TextureFilter::Linear;
    case fastgltf::Filter::LinearMipMapLinear:
      return TextureFilter::LinearMipmapLinear;
    case fastgltf::Filter::LinearMipMapNearest:
      return TextureFilter::LinearMipmapNearest;
    case fastgltf::Filter::Nearest:
      return TextureFilter::Nearest;
    case fastgltf::Filter::NearestMipMapLinear:
      return TextureFilter::NearestMipmapLinear;
    case fastgltf::Filter::NearestMipMapNearest:
      return TextureFilter::NearestMipmapNearest;
    default:
      return TextureFilter::Linear;
  }
}

TextureWrap GltfImporter::GltfWrapToTextureWrap(fastgltf::Wrap wrap) {
  switch (wrap) {
    case fastgltf::Wrap::ClampToEdge:
      return TextureWrap::Clamp;
    case fastgltf::Wrap::MirroredRepeat:
      return TextureWrap::MirrorRepeat;
    case fastgltf::Wrap::Repeat:
      return TextureWrap::Repeat;
    default:
      return TextureWrap::Repeat;
  }
}

void GltfImporter::GltfSamplerToTextureParams(TextureParams& params, fastgltf::Sampler& sampler) {
  if (sampler.magFilter.has_value()) {
    params.magFilter = GltfFilterToTextureFilter(sampler.magFilter.value()); 
  }
  if (sampler.minFilter.has_value()) {
    params.minFilter = GltfFilterToTextureFilter(sampler.minFilter.value());
  }

  params.wrapU = GltfWrapToTextureWrap(sampler.wrapS);
  params.wrapV = GltfWrapToTextureWrap(sampler.wrapT);
}

std::vector<Material*> GltfImporter::LoadMaterials(Scene* scene, fastgltf::Asset& asset, bool isSkinned) {
  std::vector<Material*> materials;
  materials.reserve(asset.materials.size());
  ResourceDatabase* resources = scene->Resources();

  const char* vertexShaderPath = isSkinned ? "./res/shaders/lit_gltf_animation.vert" : "./res/shaders/lit_gltf.vert";

	auto* opaqueProg = ShaderProgram::Build().WithVertexShader(
		resources->Get<VertexShader>(vertexShaderPath)
	).WithPixelShader(
		resources->Get<PixelShader>("./res/shaders/pbr_gltf.frag")
	).Link();

  auto* maskProg = ShaderProgram::Build().WithVertexShader(
    resources->Get<VertexShader>("./res/shaders/lit_gltf_animation.vert")
  ).WithPixelShader(
    resources->Get<PixelShader>("./res/shaders/pbr_gltf_mask.frag")
  ).Link();

  for (auto& gltfMaterial : asset.materials) {
    Material* material = nullptr;

    switch (gltfMaterial.alphaMode){
      case fastgltf::AlphaMode::Blend:
#warning GltfImporter: Add blending once engine supports semi-transparent materials 
        material = new Material(maskProg);
        material->SetValue("alphaCutoff", gltfMaterial.alphaCutoff);
        break;
      case fastgltf::AlphaMode::Opaque:
        material = new Material(opaqueProg);
        break;
      case fastgltf::AlphaMode::Mask:
        material = new Material(maskProg);
        material->SetValue("alphaCutoff", gltfMaterial.alphaCutoff);
        break;
    }

    material->name = gltfMaterial.name;

    // Diffuse
    glm::vec4 baseColorFactor = glm::make_vec4(gltfMaterial.pbrData.baseColorFactor.data());
    material->SetValue("baseColorFactor", baseColorFactor);

    if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
      std::size_t textureIndex = gltfMaterial.pbrData.baseColorTexture->textureIndex;
      if (asset.textures[textureIndex].imageIndex.has_value()) {
        std::size_t imageIndex = asset.textures[textureIndex].imageIndex.value();

        TextureParams texParams = Texture::ColorTextureRGBA;
        if (asset.textures[textureIndex].samplerIndex.has_value()) {
          auto& sampler = asset.samplers[asset.textures[textureIndex].samplerIndex.value()];
          GltfSamplerToTextureParams(texParams, sampler);
        }

        Texture2D* texture = LoadImage(scene, asset, asset.images[imageIndex], texParams);
        material->SetValue("albedoMap", texture);
      }
    } else {
      Texture2D* defaultAlbedo = resources->Get<Texture2D>("./res/textures/default_color.png", Texture::ColorTextureRGBA);
      material->SetValue("albedoMap", defaultAlbedo);
    }
   
    // Arm 
    float roughnessFactor = gltfMaterial.pbrData.roughnessFactor;
    material->SetValue("roughnessFactor", roughnessFactor);
    float metallicFactor = gltfMaterial.pbrData.metallicFactor;
    material->SetValue("metallicFactor", metallicFactor);

    if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value()) {
      std::size_t textureIndex = gltfMaterial.pbrData.metallicRoughnessTexture->textureIndex;
      if (asset.textures[textureIndex].imageIndex.has_value()) {
        std::size_t imageIndex = asset.textures[textureIndex].imageIndex.value();

        TextureParams texParams = Texture::TechnicalMapXYZ;
        if (asset.textures[textureIndex].samplerIndex.has_value()) {
          auto& sampler = asset.samplers[asset.textures[textureIndex].samplerIndex.value()];
          GltfSamplerToTextureParams(texParams, sampler);
        }

        Texture2D* texture = LoadImage(scene, asset, asset.images[imageIndex], texParams);
        material->SetValue("armMap", texture);
      }
    } else {
      Texture2D* defaultArm = resources->Get<Texture2D>("./res/textures/default_arm.png", Texture::TechnicalMapXYZ);
      material->SetValue("armMap", defaultArm);
    }

    // Occlusion
    // Assumes that the occlusion value is packed into the arm texture
    //  this just checks whether the material uses it and ignores the values if it doesn't
    if (!gltfMaterial.occlusionTexture.has_value()) {
      material->SetValue("useOcclusion", false);
      // I'm assuming the uv index for this and arm is the same???
    }
   
    // Normal
    if (gltfMaterial.normalTexture.has_value()) {
      std::size_t textureIndex = gltfMaterial.normalTexture->textureIndex;
      if (asset.textures[textureIndex].imageIndex.has_value()) {
        std::size_t imageIndex = asset.textures[textureIndex].imageIndex.value();

        TextureParams texParams = Texture::TechnicalMapXYZ;
        if (asset.textures[textureIndex].samplerIndex.has_value()) {
          auto& sampler = asset.samplers[asset.textures[textureIndex].samplerIndex.value()];
          GltfSamplerToTextureParams(texParams, sampler);
        }

        Texture2D* texture = LoadImage(scene, asset, asset.images[imageIndex], texParams);
        material->SetValue("normalMap", texture);
      }
    } else {
      Texture2D* defaultNormal = resources->Get<Texture2D>("./res/textures/default_norm.png", Texture::TechnicalMapXYZ);
      material->SetValue("normalMap", defaultNormal);
    }

    // Emissive
    glm::vec3 emissiveFactor = glm::make_vec3(gltfMaterial.emissiveFactor.data());
    material->SetValue("emissiveFactor", emissiveFactor);
    float emissiveStrength = gltfMaterial.emissiveStrength;
    material->SetValue("emissiveStrength", emissiveStrength);

    if (gltfMaterial.emissiveTexture.has_value()) {
      std::size_t textureIndex = gltfMaterial.emissiveTexture->textureIndex;
      if (asset.textures[textureIndex].imageIndex.has_value()) {
        std::size_t imageIndex = asset.textures[textureIndex].imageIndex.value();

        TextureParams texParams = Texture::ColorTextureRGB;
        if (asset.textures[textureIndex].samplerIndex.has_value()) {
          auto& sampler = asset.samplers[asset.textures[textureIndex].samplerIndex.value()];
          GltfSamplerToTextureParams(texParams, sampler);
        }

        Texture2D* texture = LoadImage(scene, asset, asset.images[imageIndex], texParams);
        material->SetValue("emissiveMap", texture);
      }
    } else {
      Texture2D* defaultEmissive = resources->Get<Texture2D>("./res/textures/default_emissive.png", Texture::ColorTextureRGB);
      material->SetValue("emissiveMap", defaultEmissive);
    }

    materials.push_back(material);

    // Add other stuff:
    //  clearcoat, culling, alpha blending
  }
  
  return materials;
}
