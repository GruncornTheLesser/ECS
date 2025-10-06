#pragma once
#include "core/fwd.h"
#include <bit>
#include <cstdint>
#include <functional>

namespace ecs {
	struct entity { // type erased handle
		using ecs_category = ecs::tag::entity;

		template<std::unsigned_integral, std::size_t> friend struct handle;

		template<std::unsigned_integral T, std::size_t N>
		constexpr entity(handle<T, N> handle) : data(static_cast<uint64_t>(handle.data)) { }

		friend constexpr bool operator==(entity lhs, entity rhs) { 
			return lhs.data == rhs.data;
		}
	private:
		uint64_t data;
	};

	template<std::unsigned_integral T, std::size_t N>
	struct handle {
		friend struct entity;
		template<typename> friend struct std::hash;
		template<typename> friend struct std::less;
		
		static constexpr std::size_t integral_width = sizeof(T) * 8;
		static constexpr std::size_t version_offset = integral_width - N;
		static constexpr T version_mask = N == 0 ? 0 : static_cast<T>(-1ull << (version_offset % integral_width));
		static constexpr T index_mask = ~version_mask;
		static constexpr T version_increment = std::bit_floor(~version_mask) << 1;

		constexpr handle() : data(index_mask) { }
		constexpr handle(tombstone) : data(index_mask) { }
		constexpr handle(entity ent) : data(ent.data) { }
		constexpr handle(uint64_t idx, uint64_t vers = 0) : data((idx & index_mask) | (vers & version_mask)) { }
		
		constexpr handle& operator++() { data += version_increment; return *this; }
		constexpr handle& operator=(tombstone) { data |= index_mask; return *this; }

		constexpr operator std::size_t() const { return data & index_mask; }

		friend constexpr bool operator==(handle lhs, tombstone rhs) { 
			return (lhs & index_mask) == index_mask;
		}

		friend constexpr bool operator==(handle lhs, handle rhs) { 
			return lhs.data == rhs.data;
		}
	
		T data;
	};
	static_assert(ecs::handle<uint32_t, 0>(tombstone{}).data == static_cast<uint32_t>(-1ul));
	static_assert(ecs::handle<uint32_t, 0>::version_increment == 0);
	static_assert((ecs::handle<uint32_t, 0>()).data == static_cast<uint32_t>(-1ul)); // 4294967295

}

namespace std {
	template<typename T, std::size_t N>
	struct hash<ecs::handle<T, N>> {
		std::size_t operator()(const ecs::handle<T, N>& val) const {
			return val.data;
		}
	};

	template<typename T, std::size_t N>
	struct less<ecs::handle<T, N>> {
		bool operator()(const ecs::handle<T, N>& lhs, const ecs::handle<T, N>& rhs) const {
			return lhs.data < rhs.data;
		}
	};
	
}