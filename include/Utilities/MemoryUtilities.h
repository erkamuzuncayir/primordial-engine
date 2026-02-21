#pragma once
#include "Utilities/Logger.h"

namespace PE::Utilities {
template <typename T>
void SafeShutdown(T *&ptr) {
	if (ptr) {
		ptr->Shutdown();
		delete ptr;
		ptr = nullptr;
		return;
	}
	PE_LOG_TRACE("Pointer is already null.");
}

template <typename T>
ERROR_CODE SafeShutdownReturnsErrorCode(T *&ptr) {
	ERROR_CODE result = ERROR_CODE::NULL_POINTER;
	if (ptr) {
		result = ptr->Shutdown();
		delete ptr;
		ptr = nullptr;
		return result;
	}
	PE_LOG_TRACE("Pointer is already null.");
	return result;
}

template <typename T>
void SafeDelete(T *&ptr) {
	if (ptr) {
		delete ptr;
		ptr = nullptr;
		return;
	}
	PE_LOG_TRACE("Pointer is already null.");
}

template <typename T>
void SafeDeleteArray(T *&ptr) {
	if (ptr) {
		delete[] ptr;
		ptr = nullptr;
		return;
	}
	PE_LOG_TRACE("Pointer is already null.");
}
}  // namespace PE::Utilities