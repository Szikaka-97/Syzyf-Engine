#pragma once

#include <string>
#include <glm/fwd.hpp>

#include <SceneComponent.h>

class ImGuiDrawable {
public:
	virtual void DrawImGui() = 0;
};

class DebugInspector : public SceneComponent {
public:
	DebugInspector(Scene* scene);

	virtual void DrawImGui();
	
	virtual int Order();
};

namespace Debug {
	template<typename T>
	bool Property(T& property, const std::string& name);
	
	template<>
	bool Property(float& property, const std::string& name);
	template<>
	bool Property(glm::vec2& property, const std::string& name);
	template<>
	bool Property(glm::vec3& property, const std::string& name);
	template<>
	bool Property(glm::vec4& property, const std::string& name);

	template<>
	bool Property(int& property, const std::string& name);
	template<>
	bool Property(glm::ivec2& property, const std::string& name);
	template<>
	bool Property(glm::ivec3& property, const std::string& name);
	template<>
	bool Property(glm::ivec4& property, const std::string& name);

	template<>
	bool Property(unsigned int& property, const std::string& name);
	template<>
	bool Property(glm::uvec2& property, const std::string& name);
	template<>
	bool Property(glm::uvec3& property, const std::string& name);
	template<>
	bool Property(glm::uvec4& property, const std::string& name);

	template<>
	bool Property(glm::mat3& property, const std::string& name);
	template<>
	bool Property(glm::mat4& property, const std::string& name);
};
