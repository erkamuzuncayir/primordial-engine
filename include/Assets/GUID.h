#pragma once
#include <cstdint>
#include <format>
#include <random>
#include <string>

#include "Utilities/Logger.h"

namespace PE::Assets {
struct GUID {
	uint64_t low  = 0;
	uint64_t high = 0;

	constexpr GUID() = default;

	[[nodiscard]] constexpr bool IsValid() const { return low != 0 || high != 0; }

	constexpr bool operator==(const GUID &other) const = default;

	static GUID Generate() {
		// thread_local ensures thread-safety and avoids seed race conditions
		thread_local std::random_device			rd;
		thread_local std::mt19937_64			engine(rd());
		std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

		GUID guid;
		guid.high = dist(engine);
		guid.low  = dist(engine);

		// RFC 4122 Variant and Version 4 (Random) masking layout
		// high: [time_low(32)] - [time_mid(16)] - [time_hi_and_version(16)]
		// low:  [clock_seq(16)] - [node(48)]

		// Set the 4 most significant bits of the time_hi_and_version field to 0100 (Version 4)
		guid.high &= 0xFFFFFFFFFFFF0FFFULL;
		guid.high |= 0x0000000000004000ULL;

		// Set the 2 most significant bits of the clock_seq_hi_and_reserved field to 10 (Variant 1)
		guid.low &= 0x3FFFFFFFFFFFFFFFULL;
		guid.low |= 0x8000000000000000ULL;

		return guid;
	}

	[[nodiscard]] std::string ToString() const {
		// Extract standard UUID fields via bit shifting and masking
		uint32_t time_low			 = (high >> 32) & 0xFFFFFFFF;
		uint16_t time_mid			 = (high >> 16) & 0xFFFF;
		uint16_t time_hi_and_version = high & 0xFFFF;

		uint16_t clock_seq = (low >> 48) & 0xFFFF;
		uint64_t node	   = low & 0xFFFFFFFFFFFFULL;

		// Format into standard 8-4-4-4-12 hexadecimal representation
		return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", time_low, time_mid, time_hi_and_version, clock_seq,
						   node);
	}

	static GUID FromString(const std::string &str) {
		std::string cleanStr = str;
		std::erase(cleanStr, '-');

		GUID guid;
		if (cleanStr.length() == 32) {
			// In RFC 4122 standard layout:
			// The first 16 hex characters represent the components of 'high' (time_low, time_mid, time_hi_and_version)
			// The last 16 hex characters represent the components of 'low' (clock_seq, node)
			guid.high = std::stoull(cleanStr.substr(0, 16), nullptr, 16);
			guid.low  = std::stoull(cleanStr.substr(16, 16), nullptr, 16);
		} else
			PE_LOG_ERROR(std::format("Invalid GUID string format: {}", str));

		return guid;
	}
};

inline constexpr GUID INVALID_GUID{};
}  // namespace PE::Assets

template <>
struct std::hash<PE::Assets::GUID> {
	size_t operator()(const PE::Assets::GUID &guid) const noexcept {
		return hash<uint64_t>{}(guid.low) ^ (hash<uint64_t>{}(guid.high) << 1);
	}
}; // namespace std