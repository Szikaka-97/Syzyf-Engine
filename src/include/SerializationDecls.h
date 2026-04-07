#pragma once

#include <Serialized.h>

class Texture2D;

template<>
void DeserializeOn<Texture2D>(volatile Texture2D* ptr, const json& json_node);

template<>
json Serialize<Texture2D>(const Texture2D* ptr);


class Cubemap;

template<>
void DeserializeOn<Cubemap>(volatile Cubemap* ptr, const json& json_node);

template<>
json Serialize<Cubemap>(const Cubemap* ptr);


#include <Shader.h>

template<>
void DeserializeOn<ShaderProgram>(volatile ShaderProgram* ptr, const json& json_node);

template<>
json Serialize<ShaderProgram>(const ShaderProgram* ptr);


class Scene;

template<>
void DeserializeOn<Scene>(volatile Scene* ptr, const json& json_node);

template<>
json Serialize<Scene>(const Scene* ptr);


class GameObject;

template<>
void DeserializeOn<GameObject>(volatile GameObject* ptr, const json& json_node);

template<>
json Serialize<GameObject>(const GameObject* ptr);


class Material;

template<>
void DeserializeOn<Material>(volatile Material* ptr, const json& json_node);

template<>
json Serialize<Material>(const Material* ptr);


#include <Light.h>

template<>
void DeserializeOn<Light>(volatile Light* ptr, const json& json_node);

template<>
json Serialize<Light>(const Light* ptr);


#include <Mesh.h>

template<>
void DeserializeOn<Mesh>(volatile Mesh* ptr, const json& json_node);

template<>
json Serialize<Mesh>(const Mesh* ptr);


#include <Camera.h>

template<>
void DeserializeOn<Camera>(volatile Camera* ptr, const json& json_node);

template<>
json Serialize<Camera>(const Camera* ptr);



template<>
void DeserializeOn<Camera::Perspective>(volatile Camera::Perspective* ptr, const json& json_node);

template<>
json Serialize<Camera::Perspective>(const Camera::Perspective* ptr);



template<>
void DeserializeOn<Camera::Orthographic>(volatile Camera::Orthographic* ptr, const json& json_node);

template<>
json Serialize<Camera::Orthographic>(const Camera::Orthographic* ptr);


class Tonemapper;

template<>
void DeserializeOn<Tonemapper>(volatile Tonemapper* ptr, const json& json_node);

template<>
json Serialize<Tonemapper>(const Tonemapper* ptr);


class ReflectionProbe;

template<>
void DeserializeOn<ReflectionProbe>(volatile ReflectionProbe* ptr, const json& json_node);

template<>
json Serialize<ReflectionProbe>(const ReflectionProbe* ptr);


class MeshRenderer;

template<>
void DeserializeOn<MeshRenderer>(volatile MeshRenderer* ptr, const json& json_node);

template<>
json Serialize<MeshRenderer>(const MeshRenderer* ptr);


class Bloom;

template<>
void DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node);

template<>
json Serialize<Bloom>(const Bloom* ptr);


class Skybox;

template<>
void DeserializeOn<Skybox>(volatile Skybox* ptr, const json& json_node);

template<>
json Serialize<Skybox>(const Skybox* ptr);

