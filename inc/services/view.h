#pragma once
#include "core/traits.h"
#include "core/meta.h"

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
			using where_type = meta::eval_t<select_type, 
				meta::filter_<traits::is_component>::template type, 
				meta::remove_if_<meta::pred<std::is_same, traits::component::get_manager>::template type, traits::component::get_manager_t<from_T>>::template type, 
				meta::rewrap_<inc>::template type>;
		};
	}

	template<ecs::traits::component_class ... Ts>
	struct inc {
		template<typename It>
		inline bool operator()(It& it) {
			return (it.reg->template pool<Ts>().contains(it->ent) && ...);
		}
	};

	template<traits::component_class ... Ts>
	struct exc {
		template<typename It>
		inline bool operator()(It& it) {
			return !(it->reg.template pool<Ts>().contains(it->ent) || ...);
		}
	};
}


namespace ecs {
	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view_iterator {
	public:
		using view_type = view<select_T, from_T, where_T, reg_T>;
		using from_type = meta::unwrap_t<from_T>;
		using entity_type = traits::component::get_entity_t<from_type>;
		using handle_type = traits::entity::get_handle_t<entity_type>;
		using manager_type = traits::component::get_manager_t<from_type>;
		using retrieve_set = meta::filter_t<select_T, meta::any_of<traits::is_entity, traits::is_data_component>::template type>;
	public:	
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using sentinel_type = view_sentinel;
		using value_type = meta::eval_t<retrieve_set, 
			meta::eval_each_<meta::eval_if_<traits::is_entity, 
				meta::eval_<traits::entity::get_handle>::template type,
				meta::eval_<traits::component::get_value, std::add_lvalue_reference>::template type
			>::template type>::template type, 
			meta::rewrap_<std::tuple>::template type>;
		
		view_iterator() : reg(nullptr), pos(-1), ent() { }
		view_iterator(reg_T* reg, std::size_t pos) : reg(reg), pos(pos), ent() { }
		view_iterator(const view_iterator& other) : reg(other.reg), pos(other.pos), ent(other.ent) { }
		view_iterator& operator=(const view_iterator& other) { reg = other.reg; pos = other.pos; ent = other.ent; }
		view_iterator(view_iterator&& other) : reg(other.reg), pos(other.pos), ent(other.ent) { }
		view_iterator& operator=(view_iterator&& other) { reg = other.reg; pos = other.pos; ent = other.ent; }

		constexpr value_type operator*() const { 
			return meta::apply_each<retrieve_set, value_type>([&]<typename T>->decltype(auto) { 
				if constexpr (traits::is_entity_v<T>) {
					auto& manager = reg->template get_resource<traits::component::get_manager_t<from_type>>();
					return manager.at(pos);
				} else {
					if constexpr (traits::is_manager_match_v<T, manager_type>) {
						auto& storage = reg->template get_resource<traits::component::get_storage_t<T>>();
						return storage.at(pos);
					}
					else {
						return reg->template get_component<T>(ent);
					} 
				}
			});
		}
		
		constexpr view_iterator& operator++() {
			while (--pos != static_cast<std::size_t>(-1)) {
				ent = reg->template get_resource<manager_type>().at(pos);
				if (valid()) return *this;
			}
			pos = static_cast<std::size_t>(-1);
			return *this;
		}
		constexpr view_iterator& operator--() {
			while (++pos != reg->template count<from_type>()) {
				ent = reg->template get_resource<manager_type>().at(pos); 
				if (valid()) return *this;
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
		bool valid() {
			return meta::apply<where_T>([&]<typename ... where_Ts>{ 
				return ([&]<typename T>->bool{ return inc{}(*this); }.template operator()<where_Ts>() && ...); 
			});
		}

		reg_T* reg;
		std::size_t pos;
		handle_type ent;
	};

	template<typename select_T, typename from_T, typename reg_T> requires (traits::is_manager_match_v<select_T, traits::component::get_manager_t<from_T>>)
	class view_iterator<select<select_T>, from<from_T>, where<>, reg_T> { 
		using base_resource = std::conditional_t<traits::is_entity_v<from_T>, traits::component::get_manager_t<from_T>, traits::component::get_storage_t<select_T>>;
		using base_iterator = traits::resource::get_value_t<base_resource>::iterator;
	public:
		using iterator_tag = std::bidirectional_iterator_tag;
		using value_type = typename std::iterator_traits<base_iterator>::value_type;
		using difference_type = typename std::iterator_traits<base_iterator>::difference_type;
		
		view_iterator() = default;
		view_iterator(reg_T* reg, std::size_t pos) : it(reg->template get_resource<base_resource>().begin() + pos) { }
		view_iterator(const view_iterator& other)  = default;
		view_iterator& operator=(const view_iterator& other) = default;
		view_iterator(view_iterator&& other) = default;
		view_iterator& operator=(view_iterator&& other) = default;

		constexpr view_iterator& operator++() { ++it; }
		constexpr view_iterator& operator--() { ++it; }
		
		constexpr view_iterator operator++(int) { auto tmp = *this; ++it; return tmp; }
		constexpr view_iterator operator--(int) { auto tmp = *this; --it; return tmp; }

		constexpr difference_type operator-(const view_iterator& other) const { return it - other.it; }

		constexpr bool operator==(const view_iterator& other) const { return it == other.it; }
		constexpr bool operator!=(const view_iterator& other) const { return it != other.it; }

	private:
		base_iterator it;
	};

	struct view_sentinel {
		constexpr bool operator==(const view_sentinel& other) const { return true; }
		constexpr bool operator!=(const view_sentinel& other) const { return false; }
		template<typename T> constexpr bool operator==(const T& other) const { return other == *this; }
		template<typename T> constexpr bool operator!=(const T& other) const { return other != *this; }
	};

	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view {
		using from_type = meta::unwrap_t<from_T>;
	public:
		using iterator = view_iterator<select_T, from_T, where_T, reg_T>;
		using const_iterator = view_iterator<meta::eval_each_t<select_T, std::add_const>, from_T, where_T, reg_T>;
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