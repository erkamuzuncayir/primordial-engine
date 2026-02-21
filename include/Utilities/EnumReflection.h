#pragma once
#include <array>
#include <optional>
#include <string_view>

namespace PE::Utilities {
template <typename TEnum>
struct EnumEntry {
	std::string_view name;
	TEnum			 value;
};

template <typename TEnum, size_t N>
constexpr std::string_view EnumToString(const std::array<EnumEntry<TEnum>, N> &map, TEnum value) {
	for (const auto &entry : map) {
		if (entry.value == value) {
			return entry.name;
		}
	}
	return "Unknown";
}

template <typename TEnum, size_t N>
constexpr std::optional<TEnum> StringToEnum(const std::array<EnumEntry<TEnum>, N> &map, std::string_view name) {
	for (const auto &entry : map) {
		if (entry.name == name) {
			return entry.value;
		}
	}
	return std::nullopt;
}
}  // namespace PE::Utilities