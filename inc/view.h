#pragma once
#include "traits.h"
#include "element.h"
#include "meta.h"

namespace ecs {
	namespace traits {
		template<typename ... select_Ts>
		struct view_builder<select<select_Ts...>>
		{
			using select_type = select<select_Ts...>;
			using from_type = from<meta::find_if_t<std::tuple<select_Ts...>, is_component>>;
			using where_type = typename view_builder<select_type, from_type>::where_type;
		};

		template<typename ... select_Ts, typename from_T>
		struct view_builder<select<select_Ts...>, from<from_T>>
		{
			using select_type = select<select_Ts...>;
			using from_type = from<from_T>;
			using where_type = meta::unique_t<meta::filter_t<include<select_Ts...>, is_component>>;
		};
	}

	template<typename ... Ts>
	struct select {
		// remove references from handle, add references to components, as util::tuple
		using const_type = select<const Ts...>;
		using value_type = element<std::conditional_t<traits::is_table_v<Ts> || traits::is_handle_v<Ts>, Ts, Ts&>...>;

		template<typename reg_T, typename handle_T>
		static inline value_type retrieve(reg_T* reg, std::size_t pos, handle_T ent) {
			return { get<std::remove_const_t<Ts>>(reg, pos, ent)... };
		}
	private:
		template<traits::component_class T, typename reg_T, typename handle_T>
		static inline T& get(reg_T* reg, std::size_t pos, handle_T ent) {
			return reg->template get_resource<traits::get_storage_t<T>>()[pos];
		}

		template<typename handle_T, typename reg_T>
		static inline handle_T get(reg_T* reg, std::size_t pos, handle_T ent) {
			return ent;
		}
	};

	template<typename T>
	struct from {
		using type = T;
	};

	template<typename ... Ts>
	struct include {
		template<typename reg_T, typename handle_T>
		static inline bool valid(reg_T* reg, handle_T ent) {
			return (reg->template pool<Ts>().contains(ent) && ...);
		}
	};

	template<typename ... Ts>
	struct exclude {
		template<typename reg_T, typename handle_T>
		static inline bool valid(reg_T* reg, handle_T ent) {
			return !(reg->template pool<Ts>().contains(ent) || ...);
		}
	};

	template<typename ... Ts>
	struct where {
		template<typename reg_T, typename handle_T>
		static inline bool valid(reg_T* reg, handle_T ent) {
			return (Ts::valid(reg, ent) && ...);
		}
	};
}


namespace ecs {
	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view_iterator {
	public:
		using view_type = view<select_T, from_T, where_T, reg_T>;
		using from_type = typename view_type::from_type;
		using table_type = typename view_type::table_type;
		using handle_type = typename view_type::handle_type;
		
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = typename select_T::value_type;
		using difference_type = std::ptrdiff_t;
		using sentinel_type = view_sentinel;
		
		view_iterator() : reg(nullptr), pos(-1), ent() { }
		view_iterator(reg_T* reg, std::size_t pos) : reg(reg), pos(pos), ent() { }
		view_iterator(const view_iterator& other) : reg(other.reg), pos(other.pos), ent(other.ent) { }
		view_iterator& operator=(const view_iterator& other) { reg = other.reg; pos = other.pos; ent = other.ent; }
		view_iterator(view_iterator&& other) : reg(other.reg), pos(other.pos), ent(other.ent) { }
		view_iterator& operator=(view_iterator&& other) { reg = other.reg; pos = other.pos; ent = other.ent; }

		constexpr value_type operator*() const { return select_T::retrieve(reg, pos, ent); }
		
		constexpr view_iterator& operator++() {
			while (--pos != static_cast<std::size_t>(-1)) {
				ent = reg->template get_resource<traits::get_manager_t<from_type>>()[pos];
				if (where_T::valid(reg, ent)) {
					return *this;
				}
			}
			pos = static_cast<std::size_t>(-1);
			return *this;
		}
		constexpr view_iterator& operator--() {
			while (++pos != reg->template count<from_type>()) {
				ent = reg->template get_resource<traits::get_manager_t<from_type>>()[pos]; 
				if (where_T::valid(reg, ent)) {
					return *this;
				}
			}
			pos = static_cast<std::size_t>(-1);
			return *this;
		}
		
		constexpr view_iterator operator++(int) { auto tmp = *this; ++pos; return tmp; }
		constexpr view_iterator operator--(int) { auto tmp = *this; --pos; return tmp; }

		constexpr difference_type operator-(const view_iterator& other) const { return pos - other.pos; }

		constexpr bool operator==(const view_iterator& other) const { return pos == other.pos; }
		constexpr bool operator!=(const view_iterator& other) const { return pos != other.pos; }

		constexpr bool operator==(const view_sentinel& other) const { return pos == static_cast<std::size_t>(-1); }
		constexpr bool operator!=(const view_sentinel& other) const { return pos != static_cast<std::size_t>(-1); }

	private:
		reg_T*      reg;
		std::size_t pos;
		handle_type ent;
	};

	struct view_sentinel {
		constexpr bool operator==(const view_sentinel& other) const { return true; }
		constexpr bool operator!=(const view_sentinel& other) const { return false; }
		template<typename T> constexpr bool operator==(const T& other) const { return other == *this; }
		template<typename T> constexpr bool operator!=(const T& other) const { return other != *this; }
	};

	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view {
	public:
		using from_type = std::remove_const_t<typename from_T::type>;
		using table_type = typename component_traits<from_type>::table_type;
		using handle_type = typename table_traits<table_type>::handle_type;

		using iterator = view_iterator<select_T, from_T, where_T, reg_T>;
		using const_iterator = view_iterator<typename select_T::const_type, from_T, where_T, reg_T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using sentinel_type = view_sentinel;

		[[nodiscard]] view(reg_T* reg) : reg(reg) { }

		[[nodiscard]] constexpr iterator begin() noexcept { return ++iterator{ reg, reg->template count<from_type>() }; }
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return ++const_iterator{ reg, reg->template count<from_type>() }; }
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return ++const_iterator{ reg, reg->template count<from_type>() }; }
		[[nodiscard]] constexpr view_sentinel end() noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel end() const noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel cend() const noexcept { return { }; }
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return --iterator{ reg, static_cast<std::size_t>(-1) }; } 
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return --const_iterator{ reg, static_cast<std::size_t>(-1) }; }
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return --const_iterator{ reg, static_cast<std::size_t>(-1) }; }
		[[nodiscard]] constexpr view_sentinel rend() noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel rend() const noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel crend() const noexcept { return { }; }
	private:
		reg_T* reg;
	};
}