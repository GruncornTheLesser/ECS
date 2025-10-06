#pragma once
#include "core/traits.h"
#include <functional>
#include <util.h>

namespace ecs {
	template<ecs::traits::component_class ... Ts>
	struct inc {
		bool operator()(const auto& it) {
			return (it->reg.template has_component<Ts>(it.ent) && ...);
		}
	};

	template<traits::component_class ... Ts>
	struct exc {
		bool operator()(const auto& it) {
			return !(it->reg.template has_component<Ts>(it.ent) || ...);
		}
	};
}

namespace ecs {
	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view_iterator {
		template<traits::component_class...> friend struct inc;
		template<traits::component_class...> friend struct exc;
		template<traits::component_class, typename> friend class pool;

	private:
		using view_type = view<select_T, from_T, where_T, reg_T>;
	
		using select_type = util::rewrap_t<select_T, std::tuple>;
		using from_type = util::unwrap_t<from_T>;
	
		using entity_type = traits::component::get_entity_t<from_type>;
		using handle_type = traits::component::get_handle_t<from_type>;
		using manager_type = traits::component::get_manager_t<from_type>;
		
		using retrieve_set = util::filter_t<select_type, traits::is_component>;
		using non_parallel_set = util::eval_t<retrieve_set, util::filter_<util::pred::disj_<ecs::traits::is_entity, util::cmp::to_<manager_type, util::cmp::is_ignore_const_same, ecs::traits::component::get_manager>::template type>::template inv>::template type>;

		using non_parallel_iterators = decltype(util::apply<util::eval_each_t<non_parallel_set, ecs::traits::component::get_storage>>([]<typename ... Ts> { return std::make_tuple(std::declval<Ts>().begin()...); }));

		
	
		static constexpr bool entity_access = util::pred::anyof_v<select_type, traits::is_entity>;
		static constexpr bool random_access = std::tuple_size_v<non_parallel_set>;
		
		
	public:	
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using sentinel_type = view_sentinel;
		
		using value_type = util::eval_each_t<select_type, util::eval_if_<traits::is_entity, util::supersede_<handle_type>::template type, util::eval_<util::propagate_const_<traits::component::get_value>::template type, std::add_lvalue_reference>::template type>::template type>;

		using reference = value_type;

		view_iterator() : reg(nullptr), pos(-1) { }
		view_iterator(reg_T* reg, std::size_t pos) : reg(reg), pos(pos){ }
		view_iterator(const view_iterator& other) : reg(other.reg), pos(other.pos) { }
		view_iterator& operator=(const view_iterator& other) { reg = other.reg; pos = other.pos;; }
		view_iterator(view_iterator&& other) : reg(other.reg), pos(other.pos) { }
		view_iterator& operator=(view_iterator&& other) { reg = other.reg; pos = other.pos; }
		
		constexpr reference operator*() const {
			handle_type hnd;
			if constexpr (entity_access || random_access) {
				hnd = reg->template pool<from_type>().at(pos);
			}

			return util::apply<select_type>([&]<typename ... Ts>() { 
				return std::make_tuple([&]<typename T>() {
					if constexpr (traits::is_entity_v<T>) {
						return hnd;
					} else if constexpr (util::cmp::is_ignore_const_same_v<from_type, T>) {
						return std::ref(reg->template pool<T>().component_at(pos));
					} else {
						return std::ref(reg->template pool<T>().get_component(hnd));
					}
				}.template operator()<Ts>()...);
			});
		}

		constexpr view_iterator& operator++() {
			while (++pos != reg->template count<from_type>()) {
				if (valid()) return *this;
			}
			pos = static_cast<std::size_t>(-1);
			return *this;
		}
		constexpr view_iterator& operator--() {
			while (--pos != static_cast<std::size_t>(-1)) {
				if (valid()) return *this;
			}
			pos = static_cast<std::size_t>(-1);
			return *this;
		}
		
		constexpr view_iterator& operator+(int offset) requires (!random_access) {
			pos += offset;
			return *this;
		}

		constexpr view_iterator& operator-(int offset) requires (!random_access) {
			pos -= offset;
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
				if constexpr (!random_access) {
					return true;
				} else {
					const handle_type& hnd = reg->template pool<from_type>().at(pos);
					return ([&]<typename T>{ 
						static constexpr std::size_t it_index = util::find_v<non_parallel_set, util::cmp::to_<T>::template type>;
						auto& indexer = reg->template get_attribute<traits::component::get_indexer_t<T>>();
						if (auto it = indexer.find(hnd); it == indexer.end()) {
							auto& storage = reg->template get_attribute<traits::component::get_storage_t<T>>();
							std::get<it_index>(its) = storage.at(it->second);
							return true;
						} else {
							return false;
						}
					}.template operator()<Ts>() && ...);
				}

			}) && util::apply<where_T>([&]<typename ... where_Ts>{ 
				return ([&]<typename T>() { return T{}(*this); }.template operator()<where_Ts>() && ...);
			});
		}

		reg_T* reg;
		std::size_t pos;
		[[no_unique_address]] non_parallel_iterators its;
	};

	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class view {
		template<typename ... Ts> friend class registry;
		
		using from_type = util::unwrap_t<from_T>;
		using iterator = view_iterator<select_T, from_T, where_T, reg_T>;
		using const_iterator = view_iterator<util::eval_each_t<select_T, std::add_const>, from_T, where_T, reg_T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using sentinel_type = view_sentinel;
	
		constexpr view(reg_T& reg) : reg(reg) { }
	public:
		[[nodiscard]] constexpr iterator begin() { return ++iterator{ &reg, static_cast<std::size_t>(-1) }; }
		[[nodiscard]] constexpr const_iterator begin() const { return ++const_iterator{ &reg, static_cast<std::size_t>(-1) }; }
		[[nodiscard]] constexpr const_iterator cbegin() const { return ++const_iterator{ &reg, static_cast<std::size_t>(-1) }; }
		[[nodiscard]] constexpr view_sentinel end() { return { }; }
		[[nodiscard]] constexpr view_sentinel end() const { return { }; }
		[[nodiscard]] constexpr view_sentinel cend() const { return { }; }
		[[nodiscard]] constexpr reverse_iterator rbegin() { return reverse_iterator{ --iterator{ &reg, reg.template count<from_type>() } }; } 
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const { return reverse_iterator{ --const_iterator{ &reg, reg.template count<from_type>()  }}; }
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const { return reverse_iterator{ --const_iterator{ &reg, reg.template count<from_type>()  }}; }
		[[nodiscard]] constexpr view_sentinel rend() { return { }; }
		[[nodiscard]] constexpr view_sentinel rend() const { return { }; }
		[[nodiscard]] constexpr view_sentinel crend() const { return { }; }
	private:
		reg_T& reg;
	};
}

namespace std {
	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	bool operator==(const std::reverse_iterator<ecs::view_iterator<select_T, from_T, where_T, reg_T>>& rev_it, ecs::view_sentinel) {
		return rev_it.base() == ecs::view_sentinel{ };
	}
}

namespace ecs {
	template<typename select_T, typename from_T, typename where_T, typename reg_T>
	class reverse_view : view<select_T, from_T, where_T, reg_T> {
	private:
		using view_type = view<select_T, from_T, where_T, reg_T>;	
	public:
		using iterator = view_type::reverse_iterator;
		using const_iterator = view_type::const_reverse_iterator;
		using reverse_iterator = view_type::iterator;
		using const_reverse_iterator = view_type::const_iterator;
		using sentinel_type = view_sentinel;
		
		[[nodiscard]] reverse_view(reg_T& reg) : view_type(reg) { }

		[[nodiscard]] constexpr iterator begin() { return view_type::rbegin(); }
		[[nodiscard]] constexpr const_iterator begin() const { return view_type::rbegin(); }
		[[nodiscard]] constexpr const_iterator cbegin() const { return view_type::crbegin(); }
		[[nodiscard]] constexpr view_sentinel end() { return { }; }
		[[nodiscard]] constexpr view_sentinel end() const { return { }; }
		[[nodiscard]] constexpr view_sentinel cend() const { return { }; }
		[[nodiscard]] constexpr reverse_iterator rbegin() { return view_type::begin(); } 
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const { return view_type::begin(); }
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const { return view_type::cbegin(); }
		[[nodiscard]] constexpr view_sentinel rend() { return { }; }
		[[nodiscard]] constexpr view_sentinel rend() const { return { }; }
		[[nodiscard]] constexpr view_sentinel crend() const { return { }; }
	};
}

