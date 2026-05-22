#pragma once

#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/format.h>

#include <glm/glm.hpp>

#include <Transform.h>

template<>
struct fmt::formatter<glm::vec3> : fmt::formatter<float> {
	template <typename Context>
	constexpr auto format(const glm::vec3& input, Context& ctx) const {
		return 
			fmt::format_to(ctx.out(), "("),
			formatter<float>::format(input.x, ctx),
			fmt::format_to(ctx.out(), ", "),
			formatter<float>::format(input.y, ctx),
			fmt::format_to(ctx.out(), ", "),
			formatter<float>::format(input.z, ctx),
			fmt::format_to(ctx.out(), ")");
	}
};

template<>
struct fmt::formatter<glm::vec4> : fmt::formatter<float> {
	template <typename Context>
	constexpr auto format(const glm::vec4& input, Context& ctx) const {
		return 
			fmt::format_to(ctx.out(), "("),
			formatter<float>::format(input.x, ctx),
			fmt::format_to(ctx.out(), ", "),
			formatter<float>::format(input.y, ctx),
			fmt::format_to(ctx.out(), ", "),
			formatter<float>::format(input.z, ctx),
			fmt::format_to(ctx.out(), ", "),
			formatter<float>::format(input.w, ctx),
			fmt::format_to(ctx.out(), ")");
	}
};

template<>
struct fmt::formatter<glm::quat> : fmt::formatter<float> {
	template <typename Context>
	auto format(const glm::quat& input, Context& ctx) const {
		format_to(ctx.out(), "(");

		formatter<float>::format(input.x, ctx);
		fmt::format_to(ctx.out(), ", ");
		formatter<float>::format(input.y, ctx);
		fmt::format_to(ctx.out(), ", ");
		formatter<float>::format(input.z, ctx);
		fmt::format_to(ctx.out(), ", ");
		formatter<float>::format(input.w, ctx);

		return format_to(ctx.out(), ")");
	}
};

template<>
struct fmt::formatter<SceneTransform::PositionAccess> : fmt::formatter<float> {
	template <typename Context>
	constexpr auto format(const SceneTransform::PositionAccess& input, Context& ctx) const {
		return fmt::format_to(
			ctx.out(),
			"{}",
			input.Value()
		);
	}
};

template<>
struct fmt::formatter<SceneTransform::RotationAccess> : fmt::formatter<float> {
	template <typename Context>
	constexpr auto format(const SceneTransform::RotationAccess& input, Context& ctx) const {
		return fmt::format_to(
			ctx.out(),
			"{}",
			glm::eulerAngles(input.Value())
		);
	}
};

template<>
struct fmt::formatter<SceneTransform::ScaleAccess> : fmt::formatter<float> {
	template <typename Context>
	constexpr auto format(const SceneTransform::ScaleAccess& input, Context& ctx) const {
		return fmt::format_to(
			ctx.out(),
			"{}",
			input.Value()
		);
	}
};

template<>
struct fmt::formatter<SceneTransform::TransformAccess> : fmt::formatter<float> {
	template <typename Context>
	constexpr auto format(const SceneTransform::TransformAccess& input, Context& ctx) const {
		return fmt::format_to(
			ctx.out(),
			"Position: {}, Rotation: {}, Scale: {}",
			input.Position(), input.Rotation(), input.Scale()
		);
	}
};