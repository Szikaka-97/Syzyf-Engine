#pragma once

#include <Serialized.h>

class Texture2D;

template<>
void Serialization::DeserializeOn<Texture2D>(volatile Texture2D* ptr, const json& json_node);

template<>
json Serialization::Serialize<Texture2D>(const Texture2D* ptr);


class Cubemap;

template<>
void Serialization::DeserializeOn<Cubemap>(volatile Cubemap* ptr, const json& json_node);

template<>
json Serialization::Serialize<Cubemap>(const Cubemap* ptr);


#include <Shader.h>

template<>
void Serialization::DeserializeOn<ShaderProgram>(volatile ShaderProgram* ptr, const json& json_node);

template<>
json Serialization::Serialize<ShaderProgram>(const ShaderProgram* ptr);


class Plane;

template<>
void Serialization::DeserializeOn<Plane>(volatile Plane* ptr, const json& json_node);

template<>
json Serialization::Serialize<Plane>(const Plane* ptr);


class Frustum;

template<>
void Serialization::DeserializeOn<Frustum>(volatile Frustum* ptr, const json& json_node);

template<>
json Serialization::Serialize<Frustum>(const Frustum* ptr);


class Scene;

template<>
void Serialization::DeserializeOn<Scene>(volatile Scene* ptr, const json& json_node);

template<>
json Serialization::Serialize<Scene>(const Scene* ptr);


class GameObject;

template<>
void Serialization::DeserializeOn<GameObject>(volatile GameObject* ptr, const json& json_node);

template<>
json Serialization::Serialize<GameObject>(const GameObject* ptr);


class LayerMask;

template<>
void Serialization::DeserializeOn<LayerMask>(volatile LayerMask* ptr, const json& json_node);

template<>
json Serialization::Serialize<LayerMask>(const LayerMask* ptr);


class Material;

template<>
void Serialization::DeserializeOn<Material>(volatile Material* ptr, const json& json_node);

template<>
json Serialization::Serialize<Material>(const Material* ptr);


#include <Light.h>

template<>
void Serialization::DeserializeOn<Light>(volatile Light* ptr, const json& json_node);

template<>
json Serialization::Serialize<Light>(const Light* ptr);


class BoundingBox;

template<>
void Serialization::DeserializeOn<BoundingBox>(volatile BoundingBox* ptr, const json& json_node);

template<>
json Serialization::Serialize<BoundingBox>(const BoundingBox* ptr);


#include <Mesh.h>

template<>
void Serialization::DeserializeOn<Mesh>(volatile Mesh* ptr, const json& json_node);

template<>
json Serialization::Serialize<Mesh>(const Mesh* ptr);


#include <Camera.h>

template<>
void Serialization::DeserializeOn<Camera>(volatile Camera* ptr, const json& json_node);

template<>
json Serialization::Serialize<Camera>(const Camera* ptr);



template<>
void Serialization::DeserializeOn<Camera::Perspective>(volatile Camera::Perspective* ptr, const json& json_node);

template<>
json Serialization::Serialize<Camera::Perspective>(const Camera::Perspective* ptr);



template<>
void Serialization::DeserializeOn<Camera::Orthographic>(volatile Camera::Orthographic* ptr, const json& json_node);

template<>
json Serialization::Serialize<Camera::Orthographic>(const Camera::Orthographic* ptr);


class Tonemapper;

template<>
void Serialization::DeserializeOn<Tonemapper>(volatile Tonemapper* ptr, const json& json_node);

template<>
json Serialization::Serialize<Tonemapper>(const Tonemapper* ptr);


class ReflectionProbe;

template<>
void Serialization::DeserializeOn<ReflectionProbe>(volatile ReflectionProbe* ptr, const json& json_node);

template<>
json Serialization::Serialize<ReflectionProbe>(const ReflectionProbe* ptr);


class MeshRenderer;

template<>
void Serialization::DeserializeOn<MeshRenderer>(volatile MeshRenderer* ptr, const json& json_node);

template<>
json Serialization::Serialize<MeshRenderer>(const MeshRenderer* ptr);


class Bloom;

template<>
void Serialization::DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node);

template<>
json Serialization::Serialize<Bloom>(const Bloom* ptr);


class Skybox;

template<>
void Serialization::DeserializeOn<Skybox>(volatile Skybox* ptr, const json& json_node);

template<>
json Serialization::Serialize<Skybox>(const Skybox* ptr);

