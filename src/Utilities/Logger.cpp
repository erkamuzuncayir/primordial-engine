#include "Utilities/Logger.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>
#include <vector>

namespace PE::Utilities {
namespace fs = std::filesystem;

LogLevel	  Logger::s_logLevel = LogLevel::Info | LogLevel::Debug | LogLevel::Warn | LogLevel::Error;
std::ofstream Logger::s_file;
bool		  Logger::s_initialized		 = false;
std::string	  Logger::s_logFilePath		 = (std::filesystem::current_path() / "engine.log").string();
size_t		  Logger::s_maxFiles		 = 1;
uint64_t	  Logger::s_maxFileSizeBytes = 5ull * 1024 * 1024;

ERROR_CODE Logger::Initialize(const std::string &logFilePath, size_t maxFiles, uint64_t maxFileSizeBytes) {
	if (s_initialized) return ERROR_CODE::ALREADY_INITIALIZED;

	s_logFilePath	   = logFilePath;
	s_maxFiles		   = (maxFiles == 0) ? 1 : maxFiles;
	s_maxFileSizeBytes = (maxFileSizeBytes == 0) ? (5ull * 1024 * 1024) : maxFileSizeBytes;

	RotateAtInitialize();

	s_file.open(s_logFilePath, std::ios::out | std::ios::trunc);
	s_initialized = s_file.is_open();

	if (!s_initialized) {
		std::cerr << "Logger::Initialize failed to open log file: " << s_logFilePath << std::endl;
	} else {
		s_file << "=== Log started: " << CurrentTimestampString() << " ===\n";
		s_file.flush();
	}
	return ERROR_CODE::OK;
}

ERROR_CODE Logger::Shutdown() {
	if (s_file.is_open()) {
		s_file.flush();
		s_file.close();
	}
	s_initialized = false;

	return ERROR_CODE::OK;
}

std::string Logger::FormatMessage(LogLevel level, const std::string &msg, const char *file, int line) {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto t	 = system_clock::to_time_t(now);

	std::tm tm;
#if defined(_WIN32)
	if (localtime_s(&tm, &t) != 0) {
		std::memset(&tm, 0, sizeof(std::tm));
	}
#else
	if (localtime_r(&t, &tm) == nullptr) {
		std::memset(&tm, 0, sizeof(std::tm));
	}
#endif

	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	oss << " ["
		<< (level == LogLevel::Debug  ? "DBG"
			: level == LogLevel::Info ? "INF"
			: level == LogLevel::Warn ? "WRN"
									  : "ERR")
		<< "] ";

	if (file) {
		const char *p	  = file;
		const char *slash = strrchr(p, '\\');
		if (!slash) slash = strrchr(p, '/');
		oss << (slash ? (slash + 1) : p) << ":" << line << " - ";
	}

	oss << msg;
	return oss.str();
}

std::string Logger::CurrentTimestampString() {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto t	 = system_clock::to_time_t(now);

	std::tm tm;
#if defined(_WIN32)
	if (localtime_s(&tm, &t) != 0) {
		std::memset(&tm, 0, sizeof(std::tm));
	}
#else
	if (localtime_r(&t, &tm) == nullptr) {
		std::memset(&tm, 0, sizeof(std::tm));
	}
#endif

	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
	return oss.str();
}

void Logger::Log(LogLevel level, const std::string &message, const char *file, int line) {
	if (!(s_logLevel & level)) return;

	std::string formatted = FormatMessage(level, message, file, line);

	if (!s_initialized) {
		s_file.open(s_logFilePath.empty() ? "engine.log" : s_logFilePath, std::ios::out | std::ios::app);
		s_initialized = s_file.is_open();
	}

	if (s_initialized && s_file.is_open()) {
		RotateIfNeededLocked();

		s_file << formatted << "\n";
		s_file.flush();
	}

	std::cerr << formatted << std::endl;
}

bool Logger::RotateAtInitialize() {
	if (s_maxFiles <= 1) {
		return true;
	}

	std::error_code ec;
	fs::path		base = s_logFilePath;

	if (!fs::exists(base, ec) || ec) return true;

	const std::string backup	 = BackupFileNameWithTimestampLocked();
	const fs::path	  backupPath = fs::path(backup);

	fs::rename(base, backupPath, ec);
	if (ec) {
		std::cerr << "Logger::RotateAtInitialize rename failed: " << ec.message() << std::endl;
		return false;
	}

	CleanupOldBackupsLocked();
	return true;
}

void Logger::RotateIfNeededLocked() {
	if (s_maxFileSizeBytes == 0 || s_maxFiles <= 1) return;

	std::error_code ec;
	const fs::path	base = s_logFilePath;

	if (!fs::exists(base, ec) || ec) return;

	if (const uint64_t sz = fs::file_size(base, ec); !ec && sz >= s_maxFileSizeBytes) {
		RotateNowLocked();
		CleanupOldBackupsLocked();
	}
}

bool Logger::RotateNowLocked() {
	if (s_file.is_open()) {
		s_file.flush();
		s_file.close();
	}

	std::error_code	  ec;
	const fs::path	  base		 = s_logFilePath;
	const std::string backup	 = BackupFileNameWithTimestampLocked();
	const fs::path	  backupPath = fs::path(backup);

	fs::rename(base, backupPath, ec);

	if (ec) std::cerr << "Logger::RotateNowLocked rename failed: " << ec.message() << std::endl;

	s_file.open(s_logFilePath, std::ios::out | std::ios::trunc);
	s_initialized = s_file.is_open();

	if (s_initialized) {
		s_file << "=== Log rotated: " << CurrentTimestampString() << " ===\n";
		s_file.flush();
	} else {
		std::cerr << "Logger::RotateNowLocked failed to open new log file" << std::endl;
		return false;
	}
	return true;
}

void Logger::CleanupOldBackupsLocked() {
	if (s_maxFiles <= 1) return;

	std::error_code ec;
	fs::path		base = s_logFilePath;
	fs::path		dir	 = base.parent_path();
	if (dir.empty()) dir = ".";

	std::string baseName = base.filename().string();

	std::vector<fs::directory_entry> backups;

	auto dirIt = fs::directory_iterator(dir, ec);
	if (ec) {
		PE_LOG_WARN("Can't find old backups!");
		return;
	}

	for (auto &entry : dirIt) {
		if (std::error_code fileEc; !entry.is_regular_file(fileEc) || fileEc) continue;

		if (std::string name = entry.path().filename().string(); name.size() > baseName.size() + 1 &&
																 name.compare(0, baseName.size(), baseName) == 0 &&
																 name[baseName.size()] == '.') {
			backups.push_back(entry);
		}
	}

	std::sort(backups.begin(), backups.end(), [](const fs::directory_entry &a, const fs::directory_entry &b) {
		std::error_code ea, eb;

		const auto ta = fs::last_write_time(a.path(), ea);
		const auto tb = fs::last_write_time(b.path(), eb);

		if (ea || eb) return a.path().string() < b.path().string();
		return ta < tb;
	});

	const size_t allowed = (s_maxFiles > 0) ? (s_maxFiles - 1) : 0;
	while (backups.size() > allowed) {
		auto toDelete = backups.front();
		fs::remove(toDelete.path(), ec);
		backups.erase(backups.begin());
	}
}

std::string Logger::BackupFileNameWithTimestampLocked() {
	const fs::path	  base	 = s_logFilePath;
	const std::string ts	 = CurrentTimestampString();
	std::string		  backup = base.string() + "." + ts;
	return backup;
}
}  // namespace PE::Utilities