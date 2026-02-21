#pragma once

#ifdef PE_D3D11
#include "Utilities/Logger.h"

namespace PE::Graphics::D3D11 {
using HRESULT_T = long;
std::string Internal_WstringToUtf8(const std::wstring &wstr);
HRESULT_T	LogOnFailedHR(const HRESULT_T hr, const Utilities::LogLevel logLevel, const std::string_view msg);

#define LOG_HR_RESULT(hr, logLevel, msg) (PE::Graphics::D3D11::LogOnFailedHR((hr), (logLevel), (msg)))

/**
 * @brief Releases COM-ish object and nulls the pointer.
 * @note noexcept - Release() should not throw.
 */
template <typename T>
void SafeRelease(T *&ptr) noexcept {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	} else
		PE_LOG_TRACE("Pointer is already null.");
}
}  // namespace PE::Graphics::D3D11
#endif