#pragma once
#include <string.h>

#include <charconv>
#include <filesystem>
#include <optional>
#include <string_view>
#include <type_traits>

#include "CommandLineArguments.h"
#include "Logger.h"
#include "StringUtilities.h"

namespace PE::Utilities {
class IOUtilities {
public:
	template <typename T>
	static std::optional<T> ParseNumber(std::string_view str) {
		static_assert(std::is_arithmetic_v<T>, "Template type must be numeric.");
		T result;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);

		if (ec == std::errc()) return result;

		return std::nullopt;
	}

	static ERROR_CODE ParseConfigIni(const std::string &filename, CommandLineArguments &args) {
		std::ifstream file(filename);
		if (!file.is_open()) {
			std::string logMessage = "INI file not found: " + filename + ". Default settings will be used.";
			PE_LOG_WARN(logMessage);
			return ERROR_CODE::FILE_NOT_FOUND;
		}

		std::string line;
		std::string currentSection;

		while (std::getline(file, line)) {
			std::string trimmedLine = String::Trim(line);

			if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#') {
				continue;
			}

			if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
				currentSection = String::Trim(trimmedLine.substr(1, trimmedLine.length() - 2));
				continue;
			}

			if (size_t equalsPos = trimmedLine.find('='); equalsPos != std::string::npos) {
				std::string key	  = String::Trim(trimmedLine.substr(0, equalsPos));
				std::string value = String::Trim(trimmedLine.substr(equalsPos + 1));

				args.isAnyArgumentSet = true;

				if (currentSection == "Engine") {
					if (key == "developerMode") {
						args.developerMode = String::ParseBool(value);
					}
				} else if (currentSection == "Graphics") {
					if (key == "api") {
						if (value == "D3D11")
							args.graphicAPI = Graphics::SupportedGraphicAPI::D3D11;
						else if (value == "Vulkan")
							args.graphicAPI = Graphics::SupportedGraphicAPI::Vulkan;
						else
							args.graphicAPI = Graphics::SupportedGraphicAPI::None;
					} else if (key == "width") {
						if (auto val = ParseNumber<uint16_t>(value); val.has_value())
							args.clientWidth = *val;
						else {
							std::string logMessage = "Invalid value for 'width' in INI file: " + value;
							PE_LOG_ERROR(logMessage);
						}
					}
				} else if (key == "height") {
					if (auto val = ParseNumber<uint16_t>(value); val.has_value())
						args.clientHeight = *val;
					else {
						std::string logMessage = "Invalid value for 'width' in INI file: " + value;
						PE_LOG_ERROR(logMessage);
					}
				} else if (key == "vsync") {
					args.enableVSync = String::ParseBool(value);
				} else if (currentSection == "Logging") {
					if (key == "enable") {
						args.enableLogging = String::ParseBool(value);
					} else if (key == "file") {
						args.logFilePath = value;
					} else if (key == "level") {
						if (auto val = ParseNumber<uint16_t>(value); val.has_value())
							args.logLevel = *val;
						else {
							std::string logMessage = "Invalid value for 'width' in INI file: " + value;
							PE_LOG_ERROR(logMessage);
						}
					}
				}
			}
		}
		file.close();
		return ERROR_CODE::OK;
	}

	static void GetCommandlineArguments(const int argc, char **argv, CommandLineArguments &args) {
		if (argc > 1) {
			args.isAnyArgumentSet = true;
		}

		for (int i = 1; i < argc; ++i) {
			std::string_view arg(argv[i]);

			// Helper lambda to safely grab the next argument without crashing
			auto getNextArg = [&]() -> std::string_view {
				if (i + 1 < argc) {
					return argv[++i];
				}
				PE_LOG_ERROR("Missing value for argument: " + std::string(arg));
				return {};
			};

			if (arg == "-dev") {
				args.developerMode = true;
			} else if (arg == "-log") {
				args.enableLogging = true;
			} else if (arg == "-logfile") {
				args.logFilePath = getNextArg();
			} else if (arg == "-loglevel") {
				if (auto val = ParseNumber<uint16_t>(getNextArg()); val.has_value()) {
					args.logLevel = *val;
				} else {
					PE_LOG_ERROR("Invalid number format for: " + std::string(arg));
				}
			} else if (arg == "-api") {
				std::string_view api = getNextArg();
				if (api == "D3D11") {
					args.graphicAPI = Graphics::SupportedGraphicAPI::D3D11;
				} else if (api == "Vulkan") {
					args.graphicAPI = Graphics::SupportedGraphicAPI::Vulkan;
				} else if (!api.empty()) {
					args.graphicAPI = Graphics::SupportedGraphicAPI::None;
					PE_LOG_ERROR("Unknown API: " + std::string(api));
				}
			} else if (arg == "-vsync") {
				args.enableVSync = true;
			} else if (arg == "-width") {
				if (auto val = ParseNumber<uint16_t>(getNextArg())) {
					args.clientWidth = *val;
				} else {
					PE_LOG_ERROR("Invalid number format for: " + std::string(arg));
				}
			} else if (arg == "-height") {
				if (auto val = ParseNumber<uint16_t>(getNextArg())) {
					args.clientHeight = *val;
				} else {
					PE_LOG_ERROR("Invalid number format for: " + std::string(arg));
				}
			} else {
				PE_LOG_ERROR("Unknown command line argument: " + std::string(arg));
			}
		}
	}

	static const std::filesystem::path &GetAssetsRoot() {
		static const std::filesystem::path root = std::filesystem::current_path() / "assets";
		return root;
	}

	static const std::filesystem::path &GetDefaultAssetsRoot() {
		static const std::filesystem::path root = std::filesystem::current_path() / "assets" / "defaults";
		return root;
	}

	static std::string GetAssetPath(const std::string &relativePath) {
		return (GetAssetsRoot() / relativePath).string();
	}

	static std::string GetDefaultAssetPath(const std::string &relativePath) {
		return (GetDefaultAssetsRoot() / relativePath).string();
	}

	static ERROR_CODE ReadBinaryFile(const std::filesystem::path &path, std::vector<char> &buffer) {
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			PE_LOG_ERROR("Failed to open file: " + path.string());
			return ERROR_CODE::IO_ERROR_OCCURRED;
		}

		const std::streampos fileSize = file.tellg();
		if (fileSize == -1) {
			PE_LOG_ERROR("Failed to read file size.");
			return ERROR_CODE::IO_ERROR_OCCURRED;
		}

		if (fileSize == 0) {
			buffer.clear();
			return ERROR_CODE::OK;
		}

		try {
			buffer.resize(fileSize);
		} catch (const std::bad_alloc &) {
			PE_LOG_ERROR("Not enough memory to load file.");
			return ERROR_CODE::IO_ERROR_OCCURRED;
		}

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		if (!file || file.gcount() != fileSize) {
			PE_LOG_ERROR("File read failed or incomplete.");
			return ERROR_CODE::IO_ERROR_OCCURRED;
		}

		return ERROR_CODE::OK;
	}
};
}  // namespace PE::Utilities