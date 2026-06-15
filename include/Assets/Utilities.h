#pragma once
#include <filesystem>
#include <span>

namespace PE::Assets {
std::string MemoryAssetPathKeyGenerator(std::string_view name);
std::string PathKeyGenerator(const std::filesystem::path &path);
std::string PathKeyGenerator(std::span<const std::filesystem::path> paths);
}  // namespace PE::Assets