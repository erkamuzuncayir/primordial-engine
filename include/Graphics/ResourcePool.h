#pragma once
#include <cstdint>
#include <utility>
#include <vector>

#include "Utilities/Logger.h"

namespace PE::Graphics {

template <typename T, typename HandleType = uint32_t>
class ResourcePool {
public:
	// Automatically determine the underlying type according to handle size (32-bit or 64-bit)
	static constexpr bool IS_64_BIT = sizeof(HandleType) == 8;
	using RawType = std::conditional_t<IS_64_BIT, uint64_t, uint32_t>;

	// 32-bit: First 20 bits index, last 12 bits Generation
	// 64-bit: First 32 bits index, last 32 bits Generation
	static constexpr RawType INDEX_MASK	   = IS_64_BIT ? static_cast<RawType>(0xFFFFFFFFuLL) : static_cast<RawType>(0xFFFFFuLL);
	static constexpr RawType GEN_SHIFT	   = IS_64_BIT ? 32 : 20;
	static constexpr RawType GEN_MASK	   = IS_64_BIT ? static_cast<RawType>(0xFFFFFFFFuLL) : static_cast<RawType>(0xFFFuLL);
	static constexpr RawType INVALID_INDEX = INDEX_MASK;

	HandleType Add(T &&item) { return HandleType{InternalAdd(std::move(item))}; }
	HandleType Add(const T &item) { return HandleType{InternalAdd(item)}; }

	void Remove(HandleType id) {
		const RawType rawHandle	  = static_cast<RawType>(id);
		const RawType sparseIndex = rawHandle & INDEX_MASK;
		const RawType generation  = (rawHandle >> GEN_SHIFT) & GEN_MASK;

		if (sparseIndex >= m_sparse.size() || m_sparse[sparseIndex].generation != generation ||
			m_sparse[sparseIndex].denseIndex == INVALID_INDEX) {
			PE_LOG_WARN("Attempted to remove invalid or already removed handle!");
			return;
		}

		const RawType denseIndex	 = m_sparse[sparseIndex].denseIndex;
		const RawType lastDenseIndex = static_cast<RawType>(m_data.size() - 1);

		// If the deleted element is not at the end of the array, swap it with the last element
		if (denseIndex != lastDenseIndex) {
			m_data[denseIndex]					   = std::move(m_data[lastDenseIndex]);
			const RawType lastSparseIndex		   = m_denseToSparse[lastDenseIndex];
			m_denseToSparse[denseIndex]		   = lastSparseIndex;
			m_sparse[lastSparseIndex].denseIndex = denseIndex;
		}

		m_data.pop_back();
		m_denseToSparse.pop_back();

		// Return the slot to the free list and increase its generation by one
		m_sparse[sparseIndex].denseIndex = m_freeHead;
		m_sparse[sparseIndex].generation  = (m_sparse[sparseIndex].generation + 1) & GEN_MASK;
		m_freeHead						   = sparseIndex;
	}

	[[nodiscard]] bool Has(HandleType id) const {
		const RawType raw_handle   = static_cast<RawType>(id);
		const RawType sparse_index = raw_handle & INDEX_MASK;
		const RawType generation   = (raw_handle >> GEN_SHIFT) & GEN_MASK;

		if (sparse_index >= m_sparse.size()) return false;
		if (m_sparse[sparse_index].generation != generation) return false;
		if (m_sparse[sparse_index].denseIndex == INVALID_INDEX) return false;

		return true;
	}

	T &Get(HandleType id) {
		if (!Has(id)) PE_LOG_FATAL("Attempted to access invalid handle!");
		const RawType raw_handle   = static_cast<RawType>(id);
		const RawType sparse_index = raw_handle & INDEX_MASK;
		return m_data[m_sparse[sparse_index].denseIndex];
	}

	const T &Get(HandleType id) const {
		if (!Has(id)) PE_LOG_FATAL("Attempted to access invalid handle!");
		const RawType raw_handle   = static_cast<RawType>(id);
		const RawType sparse_index = raw_handle & INDEX_MASK;
		return m_data[m_sparse[sparse_index].denseIndex];
	}

	std::vector<T>		 &Data() { return m_data; }
	const std::vector<T> &Data() const { return m_data; }

	void Clear() {
		m_data.clear();
		m_denseToSparse.clear();
		m_sparse.clear();
		m_freeHead = INVALID_INDEX;
	}

private:
	struct SparseSlot {
		RawType denseIndex = INVALID_INDEX;
		RawType generation	 = 0;
	};

	template <typename U>
	RawType InternalAdd(U &&item) {
		RawType sparseIndex = INVALID_INDEX;

		// Check if there is an empty slot, use it if there is one
		if (m_freeHead != INVALID_INDEX) {
			sparseIndex = m_freeHead;
			m_freeHead	 = m_sparse[sparseIndex].denseIndex;
		} else {
			// Otherwise add a new slot
			sparseIndex = static_cast<RawType>(m_sparse.size());
			if (sparseIndex == INVALID_INDEX) PE_LOG_FATAL("ResourcePool reached maximum capacity!");
			m_sparse.push_back({});
		}

		const RawType denseIndex = static_cast<RawType>(m_data.size());
		m_data.push_back(std::forward<U>(item));
		m_denseToSparse.push_back(sparseIndex);

		m_sparse[sparseIndex].denseIndex = denseIndex;

		// Create ID: Pack index and generation values according to handle size (32/64-bit)
		return sparseIndex | (m_sparse[sparseIndex].generation << GEN_SHIFT);
	}

	std::vector<T> m_data;
	std::vector<RawType>	m_denseToSparse;
	std::vector<SparseSlot> m_sparse;
	RawType					m_freeHead = INVALID_INDEX;
};
}  // namespace PE::Graphics