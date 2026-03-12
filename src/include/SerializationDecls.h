#pragma once

#include <Serialized.h>

template <typename T>
void DeserializeOn(volatile T* ptr, const json& json_node, std::vector<SerializedReference>& references) = delete;

void Deserialize(volatile void* ptr, const json& json_node, std::vector<SerializedReference>& references);

class Bloom;

template<>
void DeserializeOn<Bloom>(volatile Bloom* ptr, const json& json_node, std::vector<SerializedReference>& references);
