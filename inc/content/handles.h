#pragma once
#include "core/traits.h"
#include <cstdint>

// entity traits
namespace ecs {
	template<typename T>
	struct entity_traits<T, tag::entity> {
		using handle_type = ecs::handle<ecs::traits::entity::get_attrib_integral_t<T>, ecs::traits::entity::get_attrib_version_width_v<T>>;
		using create_event = traits::entity::get_attrib_create_event_t<T>;
		using destroy_event = traits::entity::get_attrib_destroy_event_t<T>;
		
		using factory_type = factory<T>;

		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>, factory_type, create_event, destroy_event>;
	};
}

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
		
		struct value_view {
			template<std::unsigned_integral, std::size_t> friend struct handle;
		
			inline constexpr value_view() : data(0) { }
			inline constexpr value_view(T data) : data(data) { }
			inline constexpr value_view(handle hnd) : data(hnd.data) { }

			inline constexpr operator integral_type() const { return data & value_mask; }

			inline constexpr value_view& operator++() { data += 1; return *this; }
			inline constexpr value_view operator++(int) { auto tmp = *this; data += 1; return tmp; }
			
			inline constexpr value_view& operator--() { data -= 1; return *this; }
			inline constexpr value_view operator--(int) { auto tmp = *this; data -= 1; return tmp; }
			
			inline constexpr bool operator==(value_view other) const { return !((data ^ other.data) & value_mask); }
			inline constexpr bool operator!=(value_view other) const { return (data ^ other.data) & value_mask; }

			inline constexpr bool operator==(integral_type other) const { return (data & value_mask) == other; }
			inline constexpr bool operator!=(integral_type other) const { return (data & value_mask) != other; }

			inline constexpr bool operator==(tombstone other) const { return value_mask == T{ *this }; }
			inline constexpr bool operator!=(tombstone other) const { return value_mask != T{ *this }; }
		private:
			T data;
		};
		
		struct version_view {
			template<std::unsigned_integral, std::size_t> friend struct handle;

		private:
			inline constexpr version_view(T data) : data(data) { }
		public:
			inline constexpr version_view() : data(0) { }
			inline constexpr version_view(handle hnd) : data(hnd.data) { }

			inline constexpr operator integral_type() const { return data & version_mask; }
			
			inline constexpr version_view& operator++() { data += increment; return *this; }
			inline constexpr version_view operator++(int) { auto tmp = *this; data += increment; return tmp; }
			
			inline constexpr version_view& operator--() { data -= increment; return *this; }
			inline constexpr version_view operator--(int) { auto tmp = *this; data -= increment; return tmp; }
			
			inline constexpr bool operator==(version_view other) const { 
				if constexpr (N == 0) return true;
				else return !((data ^ other.data) & version_mask); 
			}
			inline constexpr bool operator!=(version_view other) const { 
				if constexpr (N == 0) return false; 
				return (data ^ other.data) & version_mask;
			}
		private:
			T data;
		};
		
		// default to tombstone
		inline constexpr handle() : data(value_mask) { }
		inline constexpr handle(tombstone) : data(value_mask) { }
		
		inline constexpr handle(value_view val) : data(val) { }
		inline constexpr handle(value_view val, version_view vers) : data((val.data & value_mask) | (vers & version_mask)) { }

		inline constexpr explicit handle(T val) : data(val) { }
		
		inline constexpr operator T() const { return data; }

		inline constexpr handle& operator=(tombstone) { data = value_mask; return *this; }
		inline constexpr handle& operator=(value_view other) { data = version_view{ data } | other; return *this; }
		inline constexpr handle& operator=(version_view other) { data = value_view{ data } | other; return *this; }

		inline constexpr bool operator==(handle other) const { return other.data == data; }
		inline constexpr bool operator!=(handle other) const { return other.data != data; }

		inline constexpr bool operator==(version_view other) const { return  version_view(*this) == other; }
		inline constexpr bool operator!=(version_view other) const { return  version_view(*this) != other; }

		inline constexpr bool operator==(tombstone other) const { return value_mask == value_view{ *this }; }
		inline constexpr bool operator!=(tombstone other) const { return value_mask != value_view{ *this }; }
	private:
		T data;
	};

	struct entity { // type erased handle
		using ecs_tag = ecs::tag::entity;
		using integral_type = uint64_t;

		template<std::unsigned_integral T, std::size_t N>
		entity(handle<T, N> handle) : data(static_cast<integral_type>(handle.data)) { }
		entity(tombstone handle) : data(0xffffffffffffffff) { }

		template<std::unsigned_integral T, std::size_t N>
		operator handle<T, N>() const { return handle<T, N>(static_cast<T>(data)); }

		template<std::unsigned_integral T>
		explicit operator T() const { return static_cast<T>(data); }
	private:
		integral_type data;
	};

	template<ecs::traits::event_class T>
	struct event_entity {
		using ecs_tag = tag::entity;
	};
}
