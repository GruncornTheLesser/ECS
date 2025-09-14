#pragma once
#include "core/fwd.h"
#include <bit>
#include <cstdint>

namespace ecs {
	template<std::unsigned_integral T, std::size_t N>
	struct handle;
	
	template<std::unsigned_integral T, std::size_t N>
	struct index {
	private:
		template<std::unsigned_integral, std::size_t> friend struct handle;
		static constexpr T mask = ~static_cast<T>(-1ull << static_cast<std::size_t>((sizeof(T) * 8) - N));
	public:
		inline constexpr index() : data(0) { }
		inline constexpr index(T val) : data(val & mask) { }
		explicit inline constexpr index(handle<T, N> hnd) : data(hnd.data) { }

		inline constexpr operator T() const { return data & mask; }
		
		friend inline constexpr bool operator==(index lhs, index rhs) { 
			return !((lhs.data ^ rhs.data) & mask); 
		}
		friend inline constexpr bool operator==(index lhs, tombstone rhs) { 
			return mask == (lhs.data & mask); 
		}
	private:
		T data;
	};

	template<std::unsigned_integral T, std::size_t N>
	struct version {
	private:
		template<std::unsigned_integral, std::size_t> friend struct handle;
		static constexpr T mask = static_cast<T>(-1ull << static_cast<std::size_t>((sizeof(T) * 8) - N));
		static constexpr T increment = std::bit_floor(~mask) << 1;

	public:
		inline constexpr version() : data(0) { }
		inline constexpr version(T val) : data(val) { }
		explicit inline constexpr version(handle<T, N> hnd) : data(hnd.data) { }

		inline constexpr operator T() const { return data & mask; }
		
		inline constexpr version& operator++() { data += increment; return *this; }
		inline constexpr version operator++(int) { auto tmp = *this; data += increment; return tmp; }
		
		inline constexpr version& operator--() { data -= increment; return *this; }
		inline constexpr version operator--(int) { auto tmp = *this; data -= increment; return tmp; }
		
		friend inline constexpr bool operator==(version lhs, version rhs) { 
			if constexpr (N == 0) return true;
			else return !((lhs.data ^ rhs.data) & mask); 
		}
		friend inline constexpr bool operator==(version lhs, handle<T, N> rhs) { 
			if constexpr (N == 0) return false; 
			return (lhs.data ^ rhs.data) & mask;
		}
	private:
		T data;
	};

	template<std::unsigned_integral T, std::size_t N>
	struct handle {
	friend struct entity;
		template<std::unsigned_integral, std::size_t> friend struct index;
		template<std::unsigned_integral, std::size_t> friend struct version;
		
		inline constexpr handle() : data(0) { }
		
		inline constexpr handle(tombstone) : data(-1) { }
		inline constexpr handle& operator=(tombstone) { data = -1; return *this; }		
		inline constexpr handle(index<T, N> idx, version<T, N> vers) : data(idx | vers) { }

		inline constexpr handle(index<T, N> idx) : data(idx.data & index<T, N>::mask) { }
		inline constexpr handle& operator=(index<T, N> other) { data = version{ *this } | other; return *this; }
		
		inline constexpr handle(version<T, N> vers) : data(vers.data & version<T, N>::mask) { }
		inline constexpr handle& operator=(version<T, N> other) { data = index{ *this } | other; return *this; }

		inline explicit constexpr operator T() const { return data; }

		friend inline constexpr bool operator==(handle lhs, handle rhs) { 
			return lhs.data == rhs.data; 
		}

		friend inline constexpr bool operator==(handle lhs, tombstone rhs) { 
			return index{ lhs } == rhs;
		}
	private:
		T data;
	};
}

namespace ecs {
	struct entity { // type erased handle
		using ecs_category = ecs::tag::entity;
		using integral_type = uint32_t;

		template<std::unsigned_integral T, std::size_t N>
		entity(handle<T, N> handle) : data(static_cast<uint64_t>(handle.data)) { }
		entity(tombstone handle) : data(0xffffffffffffffff) { }

		template<std::unsigned_integral T, std::size_t N>
		operator handle<T, N>() const { return handle<T, N>(static_cast<T>(data)); }

		template<std::unsigned_integral T>
		explicit operator T() const { return static_cast<T>(data); }
	private:
		uint64_t data;
	};

	template<ecs::traits::event_class T>
	struct event_entity {
		using ecs_category = tag::entity;
		using create_event = void;
		using destroy_event = void;
	};
}