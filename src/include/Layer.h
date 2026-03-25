#pragma once

#include <stdint.h>

namespace Layer {
	constexpr uint8_t Default = 0;
	constexpr uint8_t UI = 1;
	constexpr uint8_t Gizmos = 1;
}

struct LayerMask {
	static constexpr uint32_t All = UINT32_MAX;

	uint32_t value;

	constexpr LayerMask(uint8_t layer):
	value(1 << layer) { }

	constexpr LayerMask(uint32_t mask):
	value(mask) { }

	constexpr LayerMask(const LayerMask& mask):
	value(mask.value) { }

	constexpr bool Test(uint8_t layer) {
		return ((1 << layer) & this->value) != 0;
	}

	constexpr LayerMask operator|(uint32_t mask) {
		LayerMask result = this->value | mask;

		return result;
	}

	constexpr LayerMask& operator|=(uint32_t mask) {
		this->value |= mask;

		return *this;
	}

	constexpr LayerMask operator&(uint32_t mask) {
		LayerMask result = this->value & mask;

		return result;
	}

	constexpr LayerMask& operator&=(uint32_t mask) {
		this->value &= mask;

		return *this;
	}

	constexpr operator uint32_t() const {
		return this->value;
	}
};
