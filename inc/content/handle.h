#pragma once
#include "core/fwd.h"
#include <bit>
#include <cstdint>
#include <functional>

namespace ecs {
	template<std::unsigned_integral T, std::size_t N>
	struct handle {
		friend struct entity;
		template<typename> friend struct std::hash;
		template<typename> friend struct std::less;

		struct index {
		private:
			template<std::unsigned_integral, std::size_t> friend struct handle;
			static constexpr T mask = ~static_cast<T>(-1ull << static_cast<std::size_t>((sizeof(T) * 8) - N));
		public:
			constexpr index() : data(0) { }
			constexpr index(std::size_t val) : data(val & mask) { }
			constexpr index(handle<T, N> hnd) : data(hnd.data) { }
	
			constexpr operator std::size_t() const { return data & mask; }
			
			friend constexpr bool operator==(index lhs, index rhs) { 
				return !((lhs.data ^ rhs.data) & mask); 
			}
			friend constexpr bool operator==(index lhs, tombstone rhs) { 
				return mask == (lhs.data & mask); 
			}
		private:
			T data;
		};
	
		struct version {
		private:
			template<std::unsigned_integral, std::size_t> friend struct handle;
			static constexpr std::size_t offset = (sizeof(T) * 8) - N;
			static constexpr T mask = static_cast<T>(-1ull << offset);
			static constexpr T increment = std::bit_floor(~mask) << 1;
	
		public:
			constexpr version() : data(0) { }
			constexpr version(std::size_t val) : data(val << offset) { }
			constexpr version(handle<T, N> hnd) : data(hnd.data) { }
	
			constexpr operator std::size_t() const { return (data & mask); }
			
			constexpr version& operator++() { data += increment; return *this; }
			constexpr version operator++(int) { auto tmp = *this; data += increment; return tmp; }
			
			friend constexpr bool operator==(version lhs, version rhs) { 
				if constexpr (N == 0) return true;
				else return !((lhs.data ^ rhs.data) & mask); 
			}
		private:
			T data;
		};
		
		constexpr handle() : data(-1) { }
		constexpr handle(T data) : data(data) { }
		constexpr handle(tombstone) : data(-1) { }
		constexpr handle& operator=(tombstone) { data = -1; return *this; }
		constexpr handle(index idx, version vers) : data(idx | vers) { }

		constexpr handle& operator=(index other) { data = version{ *this } | other; return *this; }
		constexpr handle& operator=(version other) { data = index{ *this } | other; return *this; }

		constexpr operator std::size_t() const { return index{ *this }; }

		friend constexpr bool operator==(handle lhs, handle rhs) { 
			return lhs.data == rhs.data;
		}

		friend constexpr bool operator==(handle lhs, tombstone rhs) { 
			return index{ lhs } == rhs;
		}
	private:
		T data;
	};
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

namespace ecs {
	struct entity { // type erased handle
		using ecs_category = ecs::tag::entity;

		template<std::unsigned_integral T, std::size_t N>
		entity(handle<T, N> handle) : data(static_cast<uint64_t>(handle.data)) { }
		entity(tombstone handle) : data(0xffffffffffffffff) { }

		template<std::unsigned_integral T, std::size_t N>
		operator handle<T, N>() const { return handle<T, N>(static_cast<T>(data)); }
	private:
		uint64_t data;
	};
}