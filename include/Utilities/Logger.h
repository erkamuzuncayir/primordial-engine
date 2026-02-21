#pragma once
#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>

#include "Common/Common.h"

namespace PE::Utilities {
#define PE_LOG_TRACE(msg) PE::Utilities::Logger::Trace(msg, __FILE__, __LINE__)
#define PE_LOG_DEBUG(msg) PE::Utilities::Logger::Debug(msg, __FILE__, __LINE__)
#define PE_LOG_INFO(msg)  PE::Utilities::Logger::Info(msg, __FILE__, __LINE__)
#define PE_LOG_WARN(msg)  PE::Utilities::Logger::Warn(msg, __FILE__, __LINE__)
#define PE_LOG_ERROR(msg) PE::Utilities::Logger::Error(msg, __FILE__, __LINE__)
#ifndef NDEBUG
#define PE_LOG_FATAL(msg)                                                                                              \
	do {                                                                                                               \
		PE::Utilities::Logger::Fatal(msg, __FILE__, __LINE__);                                                         \
		assert(!"FATAL ERROR! Look call stack and engine.log!");                                                       \
	} while (0)
#else
#define PE_LOG_FATAL(msg) PE::Utilities::Logger::Fatal(msg, __FILE__, __LINE__)
#endif

#define PE_LOG(error_code, msg)                                                                                        \
	do {                                                                                                               \
		const auto ec_ = (error_code);                                                                                 \
		if (ec_ < PE::ERROR_CODE::ERROR_START) {                                                                       \
			PE_LOG_FATAL(msg);                                                                                         \
		} else if (ec_ < PE::ERROR_CODE::WARN_START) {                                                                 \
			PE_LOG_ERROR(msg);                                                                                         \
		} else if (ec_ < PE::ERROR_CODE::DEBUG_START) {                                                                \
			PE_LOG_WARN(msg);                                                                                          \
		} else if (ec_ < PE::ERROR_CODE::TRACE_START) {                                                                \
			PE_LOG_DEBUG(msg);                                                                                         \
		} else if (ec_ < PE::ERROR_CODE::OK_START) {                                                                   \
			PE_LOG_TRACE(msg);                                                                                         \
		} else {                                                                                                       \
			PE_LOG_INFO(msg);                                                                                          \
		}                                                                                                              \
	} while (0)

#define PE_CHECK(var, expr)                                                                                            \
	do {                                                                                                               \
		(var) = (expr);                                                                                                \
		if ((var) < PE::ERROR_CODE::WARN_START) {                                                                      \
			return (var);                                                                                              \
		}                                                                                                              \
	} while (0)

#define PE_CHECK_ALREADY_INIT(flag, msg)                                                                               \
	do {                                                                                                               \
		if (flag) {                                                                                                    \
			PE_LOG_FATAL(msg);                                                                                         \
			return PE::ERROR_CODE::ALREADY_INITIALIZED;                                                                \
		}                                                                                                              \
	} while (0)

#define PE_CHECK_STATE_INIT(state, msg)                                                                                \
	do {                                                                                                               \
		if ((state) != PE::SystemState::Uninitialized) {                                                               \
			PE_LOG_FATAL(msg);                                                                                         \
			return PE::ERROR_CODE::ALREADY_INITIALIZED;                                                                \
		}                                                                                                              \
	} while (0)

#define PE_ENSURE_INIT(var, expr, msg)                                                                                 \
	do {                                                                                                               \
		(var) = (expr);                                                                                                \
		if ((var) < PE::ERROR_CODE::WARN_START) {                                                                      \
			PE_LOG((var), msg);                                                                                        \
			Shutdown();                                                                                                \
			return (var);                                                                                              \
		}                                                                                                              \
	} while (0)

#define PE_ENSURE_INIT_SILENT(var, expr)                                                                               \
	do {                                                                                                               \
		(var) = (expr);                                                                                                \
		if ((var) < PE::ERROR_CODE::WARN_START) {                                                                      \
			Shutdown();                                                                                                \
			return (var);                                                                                              \
		}                                                                                                              \
	} while (0)

#define PE_NULLIFY(var_1, var_2, expr)                                                                                 \
	do {                                                                                                               \
		(var_1) = (expr);                                                                                              \
		if ((var_1) < PE::ERROR_CODE::WARN_START) {                                                                    \
			SafeDelete((var_2));                                                                                       \
			(var_2) = nullptr;                                                                                         \
			return (var_1);                                                                                            \
		}                                                                                                              \
	} while (0)

enum class LogLevel : uint8_t {
	None  = 0,
	Trace = 1 << 1,
	Debug = 1 << 2,
	Info  = 1 << 3,
	Warn  = 1 << 4,
	Error = 1 << 5,
	Fatal = 1 << 6,
};

inline LogLevel operator|(LogLevel lhs, LogLevel rhs) {
	return static_cast<LogLevel>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline LogLevel operator&(LogLevel lhs, LogLevel rhs) {
	return static_cast<LogLevel>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool operator!(LogLevel level) { return static_cast<uint8_t>(level) == 0; }

class Logger {
public:
	Logger() = delete;
	// Initialize logging (call once at startup). Returns false on failure.
	// logFilePath: main filename (e.g. "engine.log").
	// maxFiles: how many files to keep including the current one. If 1, no backups.
	// maxFileSizeBytes: rotate when file size >= this many bytes.
	static ERROR_CODE Initialize(const std::string &logFilePath = "Logs/engine.log", size_t maxFiles = 5,
								 uint64_t maxFileSizeBytes = 5uLL * 1024 * 1024);  // default 5 MB

	// Shutdown logger (flush/close).
	static ERROR_CODE Shutdown();

	// Main log method
	static void Log(LogLevel level, const std::string &message, const char *file = nullptr, int line = 0);

	// Convenience wrappers
	static void Trace(const std::string &msg, const char *file = nullptr, const int line = 0) {
		Log(LogLevel::Trace, msg, file, line);
	}

	static void Debug(const std::string &msg, const char *file = nullptr, const int line = 0) {
		Log(LogLevel::Debug, msg, file, line);
	}

	static void Info(const std::string &msg, const char *file = nullptr, const int line = 0) {
		Log(LogLevel::Info, msg, file, line);
	}

	static void Warn(const std::string &msg, const char *file = nullptr, const int line = 0) {
		Log(LogLevel::Warn, msg, file, line);
	}

	static void Error(const std::string &msg, const char *file = nullptr, const int line = 0) {
		Log(LogLevel::Error, msg, file, line);
	}

	static void Fatal(const std::string &msg, const char *file = nullptr, const int line = 0) {
		Log(LogLevel::Fatal, msg, file, line);
	}

private:
	static LogLevel		 s_logLevel;
	static std::ofstream s_file;
	static bool			 s_initialized;
	static std::string	 s_logFilePath;
	static size_t		 s_maxFiles;
	static uint64_t		 s_maxFileSizeBytes;

	static std::string FormatMessage(LogLevel level, const std::string &msg, const char *file, const int line);
	static std::string CurrentTimestampString();  // YYYYMMDD_HHMMSS

	// Rotation helpers (internal)
	static bool RotateAtInitialize();		// rotate existing current log into timestamped file
	static void RotateIfNeededLocked();		// checks size and rotates (expects mutex held)
	static bool RotateNowLocked();			// perform rotation (expects mutex held)
	static void CleanupOldBackupsLocked();	// remove oldest backups to keep (maxFiles - 1)

	// utility
	static std::string BackupFileNameWithTimestampLocked();	 // base + "." + timestamp (mutex held)
};
}  // namespace PE::Utilities