#pragma once
#include <vector>

#include "Utilities/Logger.h"

namespace PE::Graphics {
template <typename T>
class ResourcePool {
public:
	uint32_t Add(T &&item) {
		const auto id = static_cast<uint32_t>(m_data.size());
		m_data.emplace_back(std::move(item));
		return id;
	}

	uint32_t Add(const T &item) {
		const auto id = static_cast<uint32_t>(m_data.size());
		m_data.push_back(item);
		return id;
	}

	bool Has(uint32_t id) {
		if (id >= m_data.size()) {
			return false;
		}
		return true;
	}

	T &Get(uint32_t id) {
		if (id >= m_data.size()) PE_LOG_FATAL("ID is bigger than or equal to size!");

		return m_data[id];
	}

	const T &Get(uint32_t id) const {
		if (id >= m_data.size()) PE_LOG_FATAL("ID is bigger than or equal to size!");

		return m_data[id];
	}

	std::vector<T>		 &Data() { return m_data; }
	const std::vector<T> &Data() const { return m_data; }
	void				  Clear() { m_data.clear(); }

private:
	std::vector<T> m_data;
};
}  // namespace PE::Graphics