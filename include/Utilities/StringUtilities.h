#pragma once
#include <algorithm>
#include <string>

namespace PE::Utilities {
namespace String {
static std::string Trim(const std::string &str) {
	const size_t first = str.find_first_not_of(" \t\n\r\f\v");
	if (std::string::npos == first) {
		return str;
	}
	const size_t last = str.find_last_not_of(" \t\n\r\f\v");
	return str.substr(first, (last - first + 1));
}

static std::string ToLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) { return std::tolower(c); });
	return s;
}

static bool ParseBool(const std::string &value) {
	const std::string lowerVal = ToLower(value);
	return (lowerVal == "true" || lowerVal == "1");
}
};	// namespace String
}  // namespace PE::Utilities