#pragma once
#include "core/fwd.h"
#include <concepts>
#include <stdint.h>
#include <bit>

namespace ecs {
	template<std::unsigned_integral T, std::size_t N>
	struct handle {
	private:
		static constexpr std::size_t version_width = N;
		static constexpr T version_mask = static_cast<T>(0xffffffffffffffffull << static_cast<std::size_t>((sizeof(T) * 8) - version_width));
		static constexpr T value_mask = ~version_mask;
		static constexpr T increment = std::bit_floor(value_mask) << 1; // version increment
	
	public:
		friend struct entity;

		using integral_type = T;
		struct value_type {
			template<std::unsigned_integral, std::size_t> friend struct handle;
		
			inline constexpr value_type() : data(0) { }
			inline constexpr value_type(T data) : data(data) { }
			inline constexpr value_type(handle hnd) : data(hnd.data) { }

			inline constexpr operator integral_type() const { return data & value_mask; }

			inline constexpr value_type& operator++() { data += 1; return *this; }
			inline constexpr value_type operator++(int) { auto tmp = *this; data += 1; return tmp; }
			
			inline constexpr value_type& operator--() { data += 1; return *this; }
			inline constexpr value_type operator--(int) { auto tmp = *this; data += 1; return tmp; }
			
			inline constexpr bool operator==(value_type other) const { return !((data ^ other.data) & version_mask); }
			inline constexpr bool operator!=(value_type other) const { return (data ^ other.data) & version_mask; }

			inline constexpr bool operator==(tombstone other) const { return value_mask == T{ *this }; }
			inline constexpr bool operator!=(tombstone other) const { return value_mask != T{ *this }; }
		private:
			T data;
		};
		struct version_type {
			template<std::unsigned_integral, std::size_t> friend struct handle;

		private:
			inline constexpr version_type(T data) : data(data) { }
		public:
			inline constexpr version_type() : data(0) { }
			inline constexpr version_type(handle hnd) : data(hnd.data) { }

			inline constexpr operator integral_type() const { return data & version_mask; }
			
			inline constexpr version_type& operator++() { data += increment; return *this; }
			inline constexpr version_type operator++(int) { auto tmp = *this; data += increment; return tmp; }
			
			inline constexpr version_type& operator--() { data += increment; return *this; }
			inline constexpr version_type operator--(int) { auto tmp = *this; data += increment; return tmp; }
			
			inline constexpr bool operator==(version_type other) const 
			{ 
				if constexpr (N == 0) return true;
				else return !((data ^ other.data) & version_mask); 
			}
			inline constexpr bool operator!=(version_type other) const { 
				if constexpr (N == 0) return false; 
				return (data ^ other.data) & version_mask;
			}
		private:
			T data;
		};
		
		// default to tombstone
		inline constexpr handle() : data(value_mask) { }
		inline constexpr handle(tombstone) : data(value_mask) { }
		
		inline constexpr handle(value_type val) : data(val) { }
		inline constexpr handle(value_type val, version_type vers) : data(static_cast<T>(val) | static_cast<T>(vers)) { }

		inline constexpr explicit handle(T val) : data(val) { }
		
		inline constexpr operator T() const { return data; }

		inline constexpr handle& operator=(tombstone) { data = value_mask; return *this; }
		inline constexpr handle& operator=(value_type other) { data = version_type{ data } | other; return *this; }
		inline constexpr handle& operator=(version_type other) { data = value_type{ data } | other; return *this; }

		inline constexpr bool operator==(handle other) const { return other.data == data; }
		inline constexpr bool operator!=(handle other) const { return other.data != data; }

		inline constexpr bool operator==(version_type other) const { return  version_type(*this) == other; }
		inline constexpr bool operator!=(version_type other) const { return  version_type(*this) != other; }

		inline constexpr bool operator==(tombstone other) const { return value_mask == value(*this); }
		inline constexpr bool operator!=(tombstone other) const { return value_mask != value(*this); }
	
	private:
		T data;
	};

	struct entity { // type erased handle
		using ecs_tag = ecs::tag::entity;
		
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

	template<typename T>
	struct event_entity {
		using ecs_tag = tag::entity;
	};
}