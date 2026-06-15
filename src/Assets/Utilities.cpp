#include "Assets/Utilities.h"

#include <format>

namespace PE::Assets {
std::string MemoryAssetPathKeyGenerator(std::string_view name) { return std::format("MEMORY_ASSET_{}", name); }

std::string PathKeyGenerator(const std::filesystem::path &path) { return path.string(); }

std::string PathKeyGenerator(const std::span<const std::filesystem::path> paths) {
	std::string pathKey;
	pathKey.reserve(paths.size() * 32);
	for (const auto &path : paths) { pathKey += path.string() + "|"; }
	return pathKey;
}
}  // namespace PE::Assets