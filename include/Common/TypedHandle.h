#pragma once
#include <cstdint>

namespace PE {
template <typename Tag, typename T = uint32_t>
struct TypedHandle {
	T value = std::numeric_limits<T>::max();

	TypedHandle() = default;
	explicit constexpr TypedHandle(T val) : value(val) {}
	explicit operator T() const { return value; }

	bool operator==(const TypedHandle& other) const { return value == other.value; }
	bool operator!=(const TypedHandle& other) const { return value != other.value; }

	[[nodiscard]] bool IsValid() const { return value != std::numeric_limits<T>::max(); }
};
}

namespace std {
template <typename Tag, typename T>
struct hash<PE::TypedHandle<Tag, T>> {
	std::size_t operator()(const PE::TypedHandle<Tag, T> &handle) const noexcept {
		return std::hash<T>{}(handle.value);
	}
};
}