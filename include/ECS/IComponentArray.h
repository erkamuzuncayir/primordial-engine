#pragma once

#include <cstdint>

#include "Common/Common.h"

namespace PE::ECS {
struct IComponentArray {
	virtual ~IComponentArray()														 = default;
	virtual ERROR_CODE			   Initialize(uint32_t maxCount)					 = 0;
	virtual ERROR_CODE			   Shutdown()										 = 0;
	virtual uint32_t			   Add(uint32_t entityId, void const *componentData) = 0;
	virtual void				   Remove(uint32_t entityId)						 = 0;
	[[nodiscard]] virtual bool	   Has(uint32_t componentIdx) const					 = 0;
	[[nodiscard]] virtual uint32_t GetCount() const									 = 0;
	virtual void				   Clear()											 = 0;
};
}  // namespace PE::ECS