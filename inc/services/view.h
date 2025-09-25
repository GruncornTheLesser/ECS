#pragma once
#include "core/traits.h"
#include <functional>
#include <util.h>

namespace ecs {
	namespace traits {
		template<typename ... select_Ts>
		struct view_builder<select<select_Ts...>> {
			using select_type = select<select_Ts...>;
			using from_type = from<util::find_t<std::tuple<select_Ts...>, is_component>>;
			using where_type = where<>;
		};

		template<typename ... select_Ts, typename from_T>
		struct view_builder<select<select_Ts...>, from<from_T>> {
			using select_type = select<select_Ts...>;
			using from_type = from<from_T>;
			using where_type = where<>;
		};
	}

	template<ecs::traits::component_class ... Ts>
	struct inc {
		inline bool operator()(const auto& it) {
			return (it->reg.template has_component<Ts>(it.ent) && ...);
		}
	};

	template<traits::component_class ... Ts>
	struct exc {
		inline bool operator()(const auto& it) {
			return !(it->reg.template has_component<Ts>(it.ent) || ...);
		}
	};
}

namespace ecs {
	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view_iterator {
		template<traits::component_class...> friend struct inc;
		template<traits::component_class...> friend struct exc;
	private:
		using view_type = view<select_T, from_T, where_T, reg_T>;
		using from_type = util::unwrap_t<from_T>;
		using entity_type = traits::component::get_entity_t<from_type>;
		using handle_type = traits::component::get_handle_t<from_type>;
		using manager_type = traits::component::get_manager_t<from_type>;
		using retrieve_set = util::filter_t<select_T, traits::is_component>;

		static constexpr bool entity_access = util::pred::anyof_v<select_T, traits::is_entity>;
		static constexpr bool random_access = util::pred::anyof_v<retrieve_set, util::cmp::to_<manager_type, util::cmp::is_ignore_const_same, traits::component::get_manager>::template type>;
		
		using non_parallel_set = util::eval_t<retrieve_set, util::filter_<util::pred::disj_<ecs::traits::is_entity, util::cmp::to_<manager_type, util::cmp::is_ignore_const_same, ecs::traits::component::get_manager>::template type>::template inv>::template type>;
		using non_parallel_iterators = decltype(std::apply([](auto&& ... args) { return std::make_tuple(args.begin()...); }, std::declval<util::eval_each_t<non_parallel_set, ecs::traits::component::get_storage>>()));
	public:	
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using sentinel_type = view_sentinel;
		
		view_iterator() : reg(nullptr), pos(-1) { }
		view_iterator(reg_T* reg, std::size_t pos) : reg(reg), pos(pos){ }
		view_iterator(const view_iterator& other) : reg(other.reg), pos(other.pos) { }
		view_iterator& operator=(const view_iterator& other) { reg = other.reg; pos = other.pos;; }
		view_iterator(view_iterator&& other) : reg(other.reg), pos(other.pos) { }
		view_iterator& operator=(view_iterator&& other) { reg = other.reg; pos = other.pos; }

		constexpr decltype(auto) operator*() const {
			handle_type hnd;
			if constexpr (entity_access || random_access) {
				hnd = reg->template get_attribute<manager_type>();
			}

			return util::apply<retrieve_set>([&]<typename ... Ts> { 
				return std::make_tuple([&]<typename T>() {
					if constexpr (traits::is_entity_v<T>) {
						return hnd;
					} else if constexpr (util::cmp::is_ignore_const_same_v<manager_type, traits::component::get_manager_t<T>>) {
						return std::ref(reg->template get_attribute<traits::component::get_storage_t<T>>().at(pos));
					} else {
						return std::ref(reg->template get_component<T>(hnd));
					}
				}.template operator()<Ts>()...);
			});
		}
		
		constexpr view_iterator& operator++() {
			while (--pos != static_cast<std::size_t>(-1)) {
				if (valid()) return *this;
			}
			pos = static_cast<std::size_t>(-1);
			return *this;
		}
		constexpr view_iterator& operator--() {
			while (++pos != reg->template count<from_type>()) {
				if (valid()) return *this;
			}
			pos = static_cast<std::size_t>(-1);
			return *this;
		}
		
		constexpr view_iterator operator++(int) { auto tmp = *this; ++pos; return tmp; }
		constexpr view_iterator operator--(int) { auto tmp = *this; --pos; return tmp; }

		constexpr difference_type operator-(const view_iterator& other) const { return pos - other.pos; }

		friend constexpr bool operator==(const view_iterator& lhs, const view_iterator& rhs) { return lhs.pos == rhs.pos; }
		friend constexpr bool operator==(const view_iterator& lhs, const view_sentinel& rhs) { return lhs.pos == static_cast<std::size_t>(-1); }

	private:
		bool valid() {
			return util::apply<non_parallel_set>([&]<typename ... Ts> {
				handle_type hnd = reg->template get_attribute<manager_type>().at(pos);
				
				return ([&]<typename T>{ 	
					auto& indexer = reg->template get_attribute<traits::component::get_indexer_t<T>>();
					if (auto it = indexer.find(hnd); it == indexer.end()) {
						auto& storage = reg->template get_attribute<traits::component::get_storage_t<T>>();
						std::get<util::find_v<non_parallel_set, util::cmp::to_<T>::template type>>(its) = storage.at(it->second);
						return true;
					} else {
						return false;
					}
				}.template operator()<Ts>() && ...);
			}) && util::apply<where_T>([&]<typename ... where_Ts>{ 
				return ([&]<typename T>() { return T{}(*this); }.template operator()<where_Ts>() && ...); 
			});
		}

		reg_T* reg;
		std::size_t pos;
		[[no_unique_address]] non_parallel_iterators its;
	};

	struct view_sentinel { };

	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view {
		using from_type = util::unwrap_t<from_T>;
	public:
		using iterator = view_iterator<select_T, from_T, where_T, reg_T>;
		using const_iterator = view_iterator<util::eval_each_t<select_T, std::add_const>, from_T, where_T, reg_T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using sentinel_type = view_sentinel;

		[[nodiscard]] view(reg_T& reg) : reg(reg) { }

		[[nodiscard]] constexpr iterator begin() noexcept { return ++iterator{ &reg, reg.template count<from_type>() }; }
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return ++const_iterator{ &reg, reg.template count<from_type>() }; }
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return ++const_iterator{ &reg, reg.template count<from_type>() }; }
		[[nodiscard]] constexpr view_sentinel end() noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel end() const noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel cend() const noexcept { return { }; }
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return --iterator{ &reg, static_cast<std::size_t>(-1) }; } 
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return --const_iterator{ &reg, static_cast<std::size_t>(-1) }; }
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return --const_iterator{ &reg, static_cast<std::size_t>(-1) }; }
		[[nodiscard]] constexpr view_sentinel rend() noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel rend() const noexcept { return { }; }
		[[nodiscard]] constexpr view_sentinel crend() const noexcept { return { }; }
	private:
		reg_T& reg;
	};
}