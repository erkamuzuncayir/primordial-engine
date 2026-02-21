#ifdef PE_D3D11
#include <filesystem>
#include <sstream>
#ifdef _WIN32
#include <comdef.h>
#endif

#include "Graphics/D3D11/D3D11Utilities.h"

namespace PE::Graphics::D3D11 {
#define FAILED_T(hr) (((HRESULT_T)(hr)) < 0)

#ifdef _WIN32
HRESULT_T LogOnFailedHR(const HRESULT_T hr, const Utilities::LogLevel logLevel, const std::string_view msg) {
	if (FAILED_T(hr)) {
		const _com_error err(hr);

		const wchar_t *wErrMsg = err.ErrorMessage();

		std::ostringstream oss;
		oss << msg.data() << "\nHR: 0x" << std::hex << hr;

		if (wErrMsg) {
			oss << " (" << Internal_WstringToUtf8(wErrMsg) << ")";
		}

		const std::string finalMsg = oss.str();

		switch (logLevel) {
			case Utilities::LogLevel::Fatal: PE_LOG_FATAL(finalMsg); break;
			case Utilities::LogLevel::Error: PE_LOG_ERROR(finalMsg); break;
			case Utilities::LogLevel::Warn: PE_LOG_WARN(finalMsg); break;
			case Utilities::LogLevel::Info: PE_LOG_INFO(finalMsg); break;
			case Utilities::LogLevel::Debug: PE_LOG_DEBUG(finalMsg); break;
			default: PE_LOG_ERROR(finalMsg); break;
		}
	}
	return hr;
}
#else
HRESULT_T LogOnFailedHR(const HRESULT_T hr, const Utilities::LogLevel logLevel, const std::string_view msg) {
	if (FAILED_T(hr)) {
		std::ostringstream oss;
		oss << msg.data() << "\nError Code: 0x" << std::hex << hr;

		const std::string finalMsg = oss.str();

		switch (logLevel) {
			case Utilities::LogLevel::Fatal: LOG_FATAL(finalMsg); break;
			case Utilities::LogLevel::Error: LOG_ERROR(finalMsg); break;
			case Utilities::LogLevel::Warn: LOG_WARN(finalMsg); break;
			case Utilities::LogLevel::Info: LOG_INFO(finalMsg); break;
			case Utilities::LogLevel::Debug: LOG_DEBUG(finalMsg); break;
			default: LOG_ERROR(finalMsg); break;
		}
	}
	return hr;
}
#endif

std::string Internal_WstringToUtf8(const std::wstring &wstr) {
	if (wstr.empty()) return std::string();

	return std::filesystem::path(wstr).string();
}
}  // namespace PE::Graphics::D3D11
#endif