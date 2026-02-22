#pragma  once
#include "fwd.h"
#include <tuple>
#include <cstddef>
#include <cassert>
#include <utility>

namespace ecs {
	template<typename> 
	struct iter_sentinel { 
		constexpr iter_sentinel() { }
		constexpr iter_sentinel(auto*, std::size_t) { }
		constexpr iter_sentinel(auto*, std::size_t, const auto&) { }
		
		constexpr iter_sentinel(const iter_sentinel&) = default;
		constexpr iter_sentinel& operator=(const iter_sentinel&) = default;
	};

	template<typename ... Ts>
	struct iter : private iter_mixin<Ts, iter<Ts...>>..., iter_mixin<typename iter_traits<iter<Ts...>>::sentinel_mixin, iter<Ts...>> {
		template<typename>           friend class pool; // to allow iterator const cast for modifiers
		template<typename...>        friend struct iter;
		template<typename, typename> friend struct iter_mixin;
		template<typename...>        friend struct iter_value;
		template<typename...>        friend struct iter_reference;
	private:
		using base_T = iter<Ts...>;
		using traits = iter_traits<base_T>;

		template<typename T>
		static constexpr auto* get_mixin(auto* base) {
			using upcast_type = base_T*;
			using downcast_type = iter_mixin<T, base_T>*;
			return static_cast<downcast_type>(static_cast<upcast_type>(base));
		}
		template<typename T>
		static constexpr const auto* get_mixin(const auto* base) {
			using upcast_type = const base_T*;
			using downcast_type = const iter_mixin<T, base_T>*;
			return static_cast<downcast_type>(static_cast<upcast_type>(base));
		}
		template<std::size_t I>
		static constexpr auto* get_mixin(auto* base) {
			return get_mixin<std::tuple_element_t<I, std::tuple<Ts...>>>(base);
		}
		template<std::size_t I>
		static constexpr const auto* get_mixin(const auto* base) {
			return get_mixin<std::tuple_element_t<I, std::tuple<Ts...>>>(base);
		}
	public:
		using iterator_category = std::random_access_iterator_tag;

		using difference_type = std::ptrdiff_t;
		using reference = iter_reference<Ts...>;
		using value_type = iter_value<Ts...>;
		
		constexpr iter() noexcept = default;
		constexpr iter(const iter&) noexcept = default;
		constexpr iter& operator=(const iter&) noexcept = default;
		
		constexpr iter(auto* container, std::size_t idx)
		 : iter(container, idx, std::type_identity<void>{}) { }
		constexpr iter(auto* container, std::size_t idx, const auto& hint)
		 : iter_mixin<Ts, base_T>(container, idx, hint)..., iter_mixin<typename traits::sentinel_mixin, base_T>(container, idx, hint) { 
			if constexpr (traits::guarded) {
				if (*this != typename traits::sentinel_mixin{} && !valid()) {
					++*this;
				}
			}
		}
		// const cast
	private:
		constexpr explicit iter(const iter<const Ts...>& other) requires(!std::is_const_v<Ts> && ...)
		 :  iter_mixin<Ts, base_T>(other)..., iter_mixin<typename traits::sentinel_mixin, base_T>(other) { }
	public:
		constexpr iter(const iter<std::remove_const_t<Ts>...>& other) requires(std::is_const_v<Ts> && ...)
		 : iter_mixin<Ts, base_T>(other)..., iter_mixin<typename traits::sentinel_mixin, base_T>(other) { }

		// dereference
		constexpr reference operator*() const noexcept { return { const_cast<iter&>(*this) }; }
		
		// modifiers
		constexpr iter& operator++() {
			if constexpr (traits::guarded) {
				do {
					[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
						++*get_mixin<typename traits::sentinel_mixin>(this);
						(++*get_mixin<Is>(this), ...);
					}(typename traits::update_sequence{});
				} while (*this != typename traits::sentinel_mixin{} && !valid());
			} else {
				[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
					(++*get_mixin<Is>(this), ...);
				}(typename traits::update_sequence{});
			}
			
			return *this;
		}

		constexpr iter& operator--() {
			if constexpr (traits::guarded) {
				do {
					[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
						--*get_mixin<iter_sentinel<typename traits::ordered_mixin>>(this);
						(--*get_mixin<Is>(this), ...);
					}(typename traits::update_sequence{});
				} while (*this != typename traits::sentinel_mixin{} && !valid());
			} else {
				[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
					(--*get_mixin<Is>(this), ...);
				}(typename traits::update_sequence{});

			}
			return *this;
		}

		constexpr iter& operator+=(difference_type n) requires (!traits::guarded) {
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) { 
				([&]{ *get_mixin<Is>(this) += n; }(), ...);
			}(typename traits::update_sequence{});
			return *this;
		}
		constexpr iter& operator-=(difference_type n) requires (!traits::guarded) { 
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				([&]{ *get_mixin<Is>(this) -= n; }(), ...);
			}(typename traits::update_sequence{});
			return *this;
		}

		constexpr iter operator++(int) { iter tmp = *this; ++*this; return tmp; }
		constexpr iter operator--(int) { iter tmp = *this; --*this; return tmp; }
		
		constexpr iter operator+(difference_type n) const requires (!traits::guarded) { auto it = *this; return it += n; }
		constexpr iter operator-(difference_type n) const requires (!traits::guarded) { auto it = *this; return it -= n; }
		
		constexpr bool valid() const { 
			return [&]<std::size_t ... Is>(std::index_sequence<Is...>) { 
				return (get_mixin<Is>(this)->valid() && ...);
			}(typename traits::guard_sequence{});
		}	

		friend constexpr difference_type operator-(const iter& lhs, const iter& rhs) requires (!traits::guarded) { 
			return *get_mixin<traits::ordered_mixin_index>(&lhs) - *get_mixin<traits::ordered_mixin_index>(&rhs);
		}
		
		friend constexpr bool operator==(const iter& lhs, const iter& rhs) {
			return *get_mixin<traits::ordered_mixin_index>(&lhs) == *get_mixin<traits::ordered_mixin_index>(&rhs);
		}

		friend constexpr auto operator<=>(const iter& lhs, const iter& rhs) {
			return *get_mixin<traits::ordered_mixin_index>(&lhs) <=> *get_mixin<traits::ordered_mixin_index>(&rhs);
		}
		
		friend constexpr void destroy(iter& it) {
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				(destroy(*get_mixin<Is>(&it)), ...);
			}(typename traits::compose_sequence{});
		}
	};
	
	template<typename ... Ts>
	struct iter_reference {
		template<typename ...> friend struct iter_value;
		template<typename ...> friend struct iter_reference;
	private:
		using base_T = iter<Ts...>;
		using traits = iter_traits<base_T>;
		using mixin_set = decltype([]<std::size_t ... Is>(std::index_sequence<Is...>){ 
			return std::type_identity<std::tuple<typename iter_mixin<std::tuple_element_t<Is, std::tuple<Ts...>>, base_T>::reference...>>{};
		}(typename traits::compose_sequence{}))::type; 

		static constexpr bool immutable_v = (std::is_const_v<Ts> && ...);
		static constexpr bool mutable_v = (!std::is_const_v<Ts> && ...);

	public:
		constexpr iter_reference(const iter_reference&) noexcept = delete;
		constexpr iter_reference& operator=(const iter_reference&) noexcept = default;

		constexpr iter_reference(iter<Ts...>& it)
		 : mixins([&]<std::size_t ... Is>(std::index_sequence<Is...>)->mixin_set { 
			return { *(*iter<Ts...>::template get_mixin<Is>(&it))... };
		}(typename traits::compose_sequence{})) { }
		
	private:
		constexpr explicit iter_reference(const iter_reference<const Ts...>& other) requires(mutable_v)
		 : mixins([&]<std::size_t ... Is>(std::index_sequence<Is...>)->mixin_set { 
			return { std::get<Is>(other.mixin)... };
		}(std::make_index_sequence<std::tuple_size_v<mixin_set>>{})) { }
	public:
		constexpr iter_reference(const iter_reference<std::remove_const_t<Ts>...>& other) requires(immutable_v)
		 : mixins([&]<std::size_t ... Is>(std::index_sequence<Is...>)->mixin_set { 
			return { std::get<Is>(other.mixin)... };
		}(std::make_index_sequence<std::tuple_size_v<mixin_set>>{})) { }

		constexpr iter_reference& operator=(iter_reference<Ts...>&& other) requires (mutable_v) {
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				((std::get<Is>(mixins) = std::move(std::get<Is>(other.mixins))), ...);
			}(std::make_index_sequence<std::tuple_size_v<mixin_set>>{});
			return *this;
		}

		constexpr iter_reference& operator=(iter_value<Ts...>&& other) requires (mutable_v) {
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				((std::get<Is>(mixins) = std::move(std::get<Is>(other.mixins))), ...);
			}(std::make_index_sequence<std::tuple_size_v<mixin_set>>{});
			return *this;
		}

		friend constexpr void swap(const iter_reference& lhs, const iter_reference& rhs) requires (mutable_v) {
			using namespace std; 
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) { 
				(swap(std::get<Is>(lhs.mixins), std::get<Is>(rhs.mixins)), ...);
			}(std::make_index_sequence<std::tuple_size_v<mixin_set>>{});
		}

		friend constexpr void swap(const iter_reference& lhs, const iter_value<Ts...>& rhs) requires (mutable_v) {
			using namespace std; 
			[&]<std::size_t ... Is>(std::index_sequence<Is...>){ 
				(swap(std::get<Is>(lhs.mixins), std::get<Is>(rhs.mixins)), ...);
			}(std::make_index_sequence<std::tuple_size_v<mixin_set>>{});
		}

		constexpr operator std::tuple_element_t<0, iter_reference<Ts...>>() {
			return get<0>(*this);
		}

		constexpr operator std::tuple_element_t<0, iter_reference<Ts...>>() const {
			return get<0>(*this);
		}

		template<std::size_t I>
		friend constexpr decltype(auto) get(const iter_reference& ref) {
			return std::as_const(std::get<traits::template exposed_compose_index<I>>(ref.mixins));
		}

		template<std::size_t I>
		friend constexpr decltype(auto) get(iter_reference& ref) {
			return std::get<traits::template exposed_compose_index<I>>(ref.mixins);
		}

		template<std::size_t I>
		friend constexpr decltype(auto) get(iter_reference&& ref) {
			return std::get<traits::template exposed_compose_index<I>>(ref.mixins);
		}

		friend constexpr bool operator==(const iter_reference& lhs, const iter_reference& rhs) {
			return std::get<traits::compare_compose_index>(lhs.mixins) == std::get<traits::compare_compose_index>(rhs.mixins);
		}
		friend constexpr auto operator<=>(const iter_reference& lhs, const iter_value<std::remove_const_t<Ts>...>& rhs) {
			return std::get<traits::compare_compose_index>(lhs.mixins) <=> std::get<traits::compare_compose_index>(rhs.mixins);
		}
		
		friend constexpr bool operator==(const iter_reference& lhs, const iter_value<std::remove_const_t<Ts>...>& rhs) { 
			return std::get<traits::compare_compose_index>(lhs.mixins) == std::get<traits::compare_compose_index>(rhs.mixins);
		}
		friend constexpr auto operator<=>(const iter_reference& lhs, const iter_reference& rhs) {
			return std::get<traits::compare_compose_index>(lhs.mixins) <=> std::get<traits::compare_compose_index>(rhs.mixins);
		}

	private:
		mixin_set mixins;
	};

	template<typename ... Ts>
	struct iter_value { 
		template<typename ...> friend struct iter_reference;

		using base_T = iter<Ts...>;
		using traits = iter_traits<base_T>;
		using mixin_set = decltype([]<std::size_t ... Is>(std::index_sequence<Is...>){ 
			return std::type_identity<std::tuple<typename iter_mixin<std::tuple_element_t<Is, std::tuple<Ts...>>, base_T>::value_type...>>{};
		}(typename traits::compose_sequence{}))::type; 

		constexpr iter_value(iter_reference<Ts...>&& other)
		 : mixins([&]<std::size_t ... Is>(std::index_sequence<Is...>)->mixin_set {
			return { std::move(std::get<Is>(other.mixins))... };
		}(std::make_index_sequence<std::tuple_size_v<mixin_set>>{})) { }

		template<std::size_t I>
		friend constexpr decltype(auto) get(iter_value<Ts...>& val) {
			return std::get<traits::template exposed_compose_index<I>>(val.mixins);
		}

		template<std::size_t I>
		friend constexpr decltype(auto) get(const iter_value<Ts...>& val) {
			return std::get<traits::template exposed_compose_index<I>>(val.mixins);
		}

		friend constexpr bool operator==(const iter_value& lhs, const iter_value& rhs) {
			return std::get<traits::compare_compose_index>(lhs.mixins) == std::get<traits::compare_compose_index>(rhs.mixins);
		}

		friend constexpr auto operator<=>(const iter_value& lhs, const iter_value& rhs) {
			return std::get<traits::compare_compose_index>(lhs.mixins) <=> std::get<traits::compare_compose_index>(rhs.mixins);
		}
	private:
		mixin_set mixins;
	};
}

// std specializations
namespace std {
	template<typename ... Ts>
	struct tuple_size<ecs::iter_reference<Ts...>>
	 : std::integral_constant<std::size_t, (ecs::tag_traits<Ts>::exposed_mixin + ... + 0)> { };

	template<typename ... Ts>
	struct tuple_size<ecs::iter_value<Ts...>> : tuple_size<ecs::iter_reference<Ts...>> { };

	template<std::size_t I, typename ... Ts>
	struct tuple_element<I, ecs::iter_reference<Ts...>>
	 : std::type_identity<typename ecs::iter_mixin<std::tuple_element_t<ecs::iter_traits<ecs::iter<Ts...>>::template exposed_mixin_index<I>, std::tuple<Ts...>>, ecs::iter<Ts...>>::reference> { };
	

	template<std::size_t I, typename ... Ts>
	struct tuple_element<I, ecs::iter_value<Ts...>>
	 : std::type_identity<typename ecs::iter_mixin<std::tuple_element_t<ecs::iter_traits<ecs::iter<Ts...>>::template exposed_mixin_index<I>, std::tuple<Ts...>>, ecs::iter<Ts...>>::value_type> { };
}

// iter mixin
namespace ecs {
	// secondary iterator mixin
	template<typename T, typename base_T>
	struct iter_mixin {
		template<typename...> friend struct iter;
		template<typename, typename> friend struct iter_mixin;
	private:
		using elem_type = T*;
		using base_type = std::conditional_t<std::is_const_v<T>, const pool<std::remove_const_t<T>>*, pool<T>*>;
	public:
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using value_type = T;
		
		// constructors
		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(auto* container, index_t idx) noexcept : iter_mixin(container, idx, std::type_identity<void>{}) { }
		constexpr iter_mixin(auto* container, index_t idx, const auto& hint) noexcept : base(std::addressof(container->template pool<std::remove_const_t<T>>())), elem(nullptr) { }
	private:
		constexpr iter_mixin(base_type base, elem_type elem) : base(base), elem(elem) { }
		// const casting
	private:
		template<typename Base_U> requires(!std::is_const_v<T>)
		constexpr iter_mixin(const iter_mixin<const T, Base_U>& other)
		 : base(const_cast<base_type>(other.base)), elem(const_cast<elem_type>(other.elem)) { }
	public:
		template<typename Base_U> requires(std::is_const_v<T>)
		constexpr iter_mixin(const iter_mixin<std::remove_const_t<T>, Base_U>& other)
		 : base(other.base), elem(other.elem) { }
		
		constexpr bool valid() const { return elem != nullptr; }
		
		// iterator functions
		constexpr reference operator*() const noexcept { return *elem; }
		
		constexpr iter_mixin& operator++() { update(); return *this; }
		constexpr iter_mixin& operator--() { update(); return *this; }

		friend constexpr void destroy(iter_mixin& mixin) {
			entity ent = **base_T::template get_mixin<entity>(&mixin);
			mixin.base->erase(ent);
		}
	private:
		constexpr void update() {
			entity ent = **base_T::template get_mixin<entity>(this);
			auto it = base->template find<T>(ent);
			elem = (it == base->template end<T>()) ? nullptr : std::addressof(get<0>(*it));
		}

		base_type base;
		elem_type elem;
	};

	// primary and entity iterator mixin
	template<typename T, typename base_T> requires(std::is_same_v<std::remove_const_t<T>, entity> || std::is_same_v<T, typename iter_traits<base_T>::primary_mixin>)
	struct iter_mixin<T, base_T> {
		template<typename...>        friend struct iter;
		template<typename, typename> friend struct iter_mixin;
	private:
		using traits = iter_traits<base_T>;

		using elem_type = T*;
		using page_type = T* const*;
		using pool_type = typename traits::primary_pool;
	public:
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using value_type = T;
		
		// constructors
	public:
		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(const iter_mixin& other) = default;
		constexpr iter_mixin& operator=(const iter_mixin& other) = default;
		constexpr iter_mixin(auto* container, std::size_t idx) noexcept
		 : iter_mixin(container, idx, std::type_identity<void>{}) { }
		constexpr iter_mixin(auto* container, std::size_t idx, const auto& hint) noexcept 
		 : iter_mixin(&container->template pool<typename traits::from_type>(), idx, hint) { }
		
		constexpr iter_mixin(pool_type* pool, std::size_t idx, const auto& hint) noexcept {
			if constexpr (std::is_same_v<std::remove_const_t<T>, entity> && requires { hint.entity_page_pointer; }) {
				page = hint.entity_page_pointer;
			}
			else if constexpr (!std::is_same_v<std::remove_const_t<T>, entity> && requires { hint.primary_page_pointer; }) {
				page = hint.primary_page_pointer;
			}
			else if constexpr (requires { hint.page_index; }) {
				page = pool->template data<T>() + hint.page_index;
			} 
			else {
				page = pool->template data<T>() + (idx / page_size);
			} 

			if constexpr (std::is_same_v<std::remove_const_t<T>, entity> && requires { hint.entity_pointer; }) {
				elem = hint.entity_pointer;
			}
			else if constexpr (!std::is_same_v<std::remove_const_t<T>, entity> && requires { hint.primary_pointer; }) {
				page = hint.primary_pointer;
			}
			else if constexpr (requires { hint.elem_index; }) {
				elem = *page + hint.elem_index;
			} 
			else {
				elem = *page + (idx % page_size);
			} 
		}
	private:
		constexpr iter_mixin(page_type page, elem_type elem) : page(page), elem(elem) { }

		// const casting
	private:
		template<typename Base_U>
		constexpr explicit iter_mixin(const iter_mixin<const T, Base_U>& other) requires(!std::is_const_v<T>)
		 : page(const_cast<page_type>(other.page)), elem(const_cast<elem_type>(other.elem)) { }
	public:
		template<typename Base_U>
		constexpr explicit iter_mixin(const iter_mixin<std::remove_const_t<T>, Base_U>& other) requires(std::is_const_v<T>)
		 : page(other.page), elem(other.elem) { }

		// iterator functions
		constexpr reference operator*() const noexcept { 
			return *elem;
		}
		
		constexpr iter_mixin& operator++() {
			if (elem == *page + page_size - 1) [[unlikely]] { // if end of page
				elem = *++page;
			}
			else {
				++elem;
			}
			
			return *this;
		}

		constexpr iter_mixin& operator--() {
			if (elem == *page) [[unlikely]] { // if end of page
				elem = *--page + page_size - 1;
			}
			else { 
				--elem;
			}
			
			return *this;
		}

		constexpr iter_mixin operator++(int) { iter_mixin tmp = *this; ++*this; return tmp; }
		constexpr iter_mixin operator--(int) { iter_mixin tmp = *this; --*this; return tmp; }
		
		constexpr iter_mixin& operator+=(difference_type n) { return *this = (*this + n); }
		constexpr iter_mixin& operator-=(difference_type n) { return *this = (*this - n); }

		constexpr iter_mixin operator+(difference_type n) const
		{
			n += elem - *page;

			page_type new_page = page + (n / page_size);
			elem_type new_elem = *new_page + (n % page_size);

			return { new_page, new_elem };
		}
		constexpr iter_mixin operator-(difference_type n) const { return *this + (-n); }

		friend constexpr difference_type operator-(const iter_mixin& lhs, const iter_mixin& rhs) {
			return (lhs.page - rhs.page) * page_size + (*lhs.page - lhs.elem) - (*rhs.page - rhs.elem);
		}
		
		friend constexpr bool operator==(const iter_mixin& lhs, const iter_mixin& rhs) {
			return lhs.elem == rhs.elem;
		}
		
		friend constexpr auto operator<=>(const iter_mixin& lhs, const iter_mixin& rhs) {
			return lhs.page == rhs.page ? lhs.elem <=> rhs.elem : lhs.page <=> rhs.page;
		}
		
		friend constexpr void destroy(iter_mixin& mixin) {
			std::destroy_at(mixin.elem);
		}
	private:
		page_type page;
		elem_type elem;
	};

	// indirect iterator mixin
	template<typename T, typename base_T> requires(std::is_same_v<std::remove_const_t<T>, indirect>)
	struct iter_mixin<T, base_T> {
		template<typename...> friend struct iter;
		template<typename, typename> friend struct iter_mixin;
	private:
		using traits = iter_traits<base_T>;
		using page_type = T* const*; // indirect
		using elem_type = T*;
		using pool_type = typename traits::primary_pool;
	public:
		using difference_type = std::ptrdiff_t;
		struct reference;
		struct value_type;

		struct reference {
			friend struct value_type;
			
			constexpr reference(elem_type ptr, index_t idx) : ptr(ptr), idx(idx) { }
			constexpr reference(const reference& other) = default;
			constexpr reference& operator=(const reference& other) = default;
			
			constexpr reference(reference&& other) : ptr(other.ptr), idx(other.idx) { }
			constexpr reference& operator=(reference&& other) requires(!std::is_const_v<T>) {
				ptr = other.ptr;
				ptr->index = idx;
				other.ptr = nullptr;
				return *this;
			}
			constexpr reference& operator=(value_type&& other) requires(!std::is_const_v<T>) {
				ptr = other.ptr;
				ptr->index = idx;
				other.ptr = nullptr;
				return *this;
			}
			friend constexpr void swap(reference lhs, reference rhs) requires(!std::is_const_v<T>) {
				if (lhs.idx == rhs.idx) return;
				
				std::swap(lhs.ptr, rhs.ptr);
				
				rhs.ptr->index = rhs.idx;
				lhs.ptr->index = lhs.idx;
			}

			friend constexpr void swap(reference lhs, value_type& rhs) requires(!std::is_const_v<T>) {
				std::swap(lhs, rhs);
			}
		private:
			elem_type ptr;
			index_t   idx;
		};

		struct value_type {
			friend struct reference;

			constexpr value_type(reference&& ref) : ptr(ref.ptr) { }
			constexpr value_type(value_type&& ref) : ptr(ref.ptr) { }
			constexpr value_type& operator=(reference&& ref) { 
				ptr = ref.ptr;
				ref.ptr = nullptr;
				return *this;
			}
			constexpr value_type& operator=(value_type&& other) {
				ptr = other.ptr;
				return *this;
			}
		private:
			elem_type ptr;
		};

		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(auto* view, index_t idx) noexcept : iter_mixin(view, idx, std::type_identity<void>{ }) { }
		constexpr iter_mixin(auto* view, index_t idx, const auto& hint) noexcept : iter_mixin(&view->template pool<typename traits::from_type>(), idx, std::type_identity<void>{ }) { }
		constexpr iter_mixin(pool_type* pool, index_t idx, const auto& hint) noexcept : pages(pool->template data<indirect>()), idx(static_cast<index_t>(idx))
		{
			if constexpr (requires { hint.indirect_pointer; }) {
				ptr = hint.indirect_pointer;
			} else {
				ptr = nullptr;
			}
		}
	private:
		template<typename Base_U> requires(!std::is_const_v<T>)
		constexpr explicit iter_mixin(const iter_mixin<const indirect, Base_U>& other)
		 : pages(const_cast<page_type>(other.pages)), ptr(const_cast<elem_type>(other.ptr)), idx(other.idx) { }
	public:
		template<typename Base_U> requires(std::is_const_v<T>)
		constexpr iter_mixin(const iter_mixin<indirect, Base_U>& other)
		 : pages(other.pages), ptr(other.ptr), idx(other.idx) { }

		constexpr reference operator*() const noexcept { 
			if (ptr == nullptr) {
				const auto& entity_mixin = *base_T::template get_mixin<entity>(this);
				auto ent = entity_mixin.elem->index;
				ptr = &pages[ent / page_size][ent % page_size];
			}
			return { ptr, idx };
		}

		constexpr iter_mixin& operator++() { return *this += 1; }
		constexpr iter_mixin& operator--() { return *this -= 1; }

		constexpr iter_mixin& operator+=(difference_type n) {
			idx += n;
			ptr = nullptr;
			return *this;
		}
		constexpr iter_mixin& operator-=(difference_type n) { 
			idx -= n;
			ptr = nullptr;
			return *this;
		}

		friend constexpr difference_type operator-(const iter_mixin& lhs, const iter_mixin& rhs) { return lhs.idx - rhs.idx; }
		friend constexpr bool operator==(const iter_mixin& lhs, const iter_mixin& rhs) { return lhs.idx == rhs.idx; }
		friend constexpr auto operator<=>(const iter_mixin& lhs, const iter_mixin& rhs) { return lhs.idx <=> rhs.idx; }
		friend constexpr void destroy(iter_mixin& mixin) {
			if (mixin.ptr == nullptr) {
				const auto& entity_mixin = *base_T::template get_mixin<entity>(&mixin);
				auto ent = entity_mixin.elem->index;
				mixin.ptr = &mixin.pages[ent / page_size][ent % page_size];
			}
			*mixin.ptr = indirect{ };
		}
	private:		
		page_type pages;
		mutable elem_type ptr;
		index_t   idx;
	};

	// pointer iterator mixin
	template<typename T_ptr, typename base_T> requires (std::is_pointer_v<T_ptr>)
	struct iter_mixin<T_ptr, base_T> {
		template<typename, typename> friend struct iter_mixin;
	private:
		using T = std::remove_pointer_t<T_ptr>;

		using elem_type = T*;
		using base_type = std::conditional_t<std::is_const_v<T>, const pool<std::remove_const_t<T>>*, pool<T>*>;
	public:
		using difference_type = std::ptrdiff_t;
		struct value_type;
		struct reference;

		struct reference {
			constexpr reference(elem_type ptr) : ptr(ptr) { }
			constexpr reference(const reference& other) = default;
			constexpr reference& operator=(const reference& other) = default;

			constexpr operator T_ptr() { return ptr; }
			constexpr operator const T_ptr() const { return ptr; }
			constexpr T_ptr operator->() const { return ptr; }

			constexpr reference(reference&& other) : ptr(other.ptr) { }
			constexpr reference& operator=(reference&& other) requires(!std::is_const_v<T>) { return *this; }
			constexpr reference& operator=(value_type&& other) requires(!std::is_const_v<T>) { return *this; }
			friend constexpr void swap(reference lhs, reference rhs) requires(!std::is_const_v<T>) { }
			friend constexpr void swap(reference lhs, value_type& rhs) requires(!std::is_const_v<T>) { }
		private:
			T_ptr ptr;
		};
		struct value_type { 
			constexpr value_type(reference&& ref) { }
			constexpr value_type(value_type&& val) { }
			constexpr ~value_type() { }
			constexpr value_type& operator=(reference&& ref) { return *this; }
			constexpr value_type& operator=(value_type&& other) { return *this; }	
		private:
			// bool value_stored;
			// union { T value; };
		};
				
		// constructors
		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(auto* view, index_t idx) noexcept : iter_mixin(view, idx, std::type_identity<void>{}) { }
		constexpr iter_mixin(auto* view, index_t idx, const auto& hint) noexcept : base(std::addressof(view->template pool<std::remove_const_t<T>>())), elem(nullptr) { }
	private:
		constexpr iter_mixin(base_type base, elem_type elem) : base(base), elem(elem) { }
		// const casting
	private:
		template<typename Base_U> requires(!std::is_const_v<T>)
		constexpr iter_mixin(const iter_mixin<const T, Base_U>& other)
		 : base(const_cast<base_type>(other.base)), elem(const_cast<elem_type>(other.elem)) { }
	public:
		template<typename Base_U> requires(std::is_const_v<T>)
		constexpr iter_mixin(const iter_mixin<std::remove_const_t<T>, Base_U>& other)
		 : base(other.base), elem(other.elem) { }
		
		// iterator functions
		constexpr reference operator*() const noexcept { return { elem }; }
		
		constexpr iter_mixin& operator++() { update(); return *this; }
		constexpr iter_mixin& operator--() { update(); return *this; }

		friend constexpr void destroy(iter_mixin& mixin) { }
	private:
		constexpr void update() {
			entity ent = **base_T::template get_mixin<entity>(this);
			auto it = base->template find<T>(ent);
			elem = (it == base->template end<T>()) ? nullptr : std::addressof(get<0>(*it));
		}

		base_type base;
		elem_type elem;
	};

	// from iterator mixin
	template<template<typename...> typename Tp, typename T, typename base_T> requires (std::is_same_v<std::remove_const_t<Tp<T>>, from<T>>)
	struct iter_mixin<Tp<T>, base_T> {
		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(const iter_mixin& other) = default;
		constexpr iter_mixin& operator=(const iter_mixin& other) = default;

		constexpr iter_mixin(auto* container, std::size_t idx) noexcept : iter_mixin(container, idx, std::type_identity<void>{}) { }
		constexpr iter_mixin(auto* container, std::size_t idx, const auto& hint) noexcept { }
	private:
		template<typename Base_U>
		constexpr explicit iter_mixin(const iter_mixin<const T, Base_U>& other) requires(!std::is_const_v<T>) { }
	public:
		template<typename Base_U>
		constexpr explicit iter_mixin(const iter_mixin<std::remove_const_t<T>, Base_U>& other) requires(std::is_const_v<T>) { }
		
		friend constexpr void destroy(iter_mixin& mixin) { }
	};

	// inc pred iterator mixin
	template<template<typename...> typename Tp, typename ... Ts, typename base_T> requires (std::is_same_v<std::remove_const_t<Tp<Ts...>>, inc<Ts...>>)
	struct iter_mixin<Tp<Ts...>, base_T> { 
		using reference = std::type_identity<void>;
		using value_type = std::type_identity<void>;

		// constructors
		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(const iter_mixin& other) = default;
		constexpr iter_mixin& operator=(const iter_mixin& other) = default;

		constexpr iter_mixin(auto* container, index_t idx) noexcept : iter_mixin(container, idx, std::type_identity<void>{}) { }
		constexpr iter_mixin(auto* container, index_t idx, const auto& hint) noexcept : pools(std::addressof(container->template pool<std::remove_const_t<Ts>>())...) { }
		
		constexpr bool valid() const { 
			entity ent = **base_T::template get_mixin<entity>(this);
			return (std::get<const pool<Ts>*>(pools)->contains(ent) && ...);
		}
	private:
		std::tuple<const pool<Ts>*...> pools;
	};

	// exc pred iterator mixin
	template<template<typename...> typename Tp, typename ... Ts, typename base_T> requires (std::is_same_v<std::remove_const_t<Tp<Ts...>>, exc<Ts...>>)
	struct iter_mixin<Tp<Ts...>, base_T> { 
		using reference = std::type_identity<void>;
		using value_type = std::type_identity<void>;

		// constructors
		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(const iter_mixin& other) = default;
		constexpr iter_mixin& operator=(const iter_mixin& other) = default;

		constexpr iter_mixin(auto* container, index_t idx) noexcept : iter_mixin(container, idx, std::type_identity<void>{}) { }
		constexpr iter_mixin(auto* container, index_t idx, const auto& hint) noexcept : pools(std::addressof(container->template pool<std::remove_const_t<Ts>>())...) { }
		
		constexpr bool valid() const { 
			entity ent = **base_T::template get_mixin<entity>(this);
			return (!std::get<const pool<Ts>*>(pools)->contains(ent) && ...);
		}
	private:
		std::tuple<const pool<Ts>*...> pools;
	};

	// any pred iterator mixin
	template<template<typename...> typename Tp, typename ... Ts, typename base_T> requires (std::is_same_v<std::remove_const_t<Tp<Ts...>>, any<Ts...>>)
	struct iter_mixin<Tp<Ts...>, base_T> { 
		// constructors
		constexpr iter_mixin() noexcept = default;
		constexpr iter_mixin(const iter_mixin& other) = default;
		constexpr iter_mixin& operator=(const iter_mixin& other) = default;

		constexpr iter_mixin(auto* container, index_t idx) noexcept : iter_mixin(container, idx, std::type_identity<void>{}) { }
		constexpr iter_mixin(auto* container, index_t idx, const auto& hint) noexcept : pools(std::addressof(container->template pool<std::remove_const_t<Ts>>())...) { }
		
		constexpr bool valid() const {
			entity ent = **base_T::template get_mixin<entity>(this);
			return (std::get<const pool<Ts>*>(pools)->contains(ent) || ...);
		}
	private:
		std::tuple<const pool<Ts>*...> pools;
	};
	
	template<template<id> typename Tp, id F, typename base_T> requires (std::is_same_v<std::remove_const_t<Tp<F>>, flag<F>>)
	struct iter_mixin<Tp<F>, base_T> { };
	
	template<template<auto> typename Tp, auto S, typename base_T> requires (std::is_same_v<std::remove_const_t<Tp<S>>, state<S>>)
	struct iter_mixin<Tp<S>, base_T> { };
	
	// template<typename T, id ID, typename base_T>
	template<template<typename, id> typename Tp, typename T, id ID, typename base_T> requires (std::is_same_v<std::remove_const_t<Tp<T, ID>>, res<T, ID>>)
	struct iter_mixin<Tp<T, ID>, base_T> { };
}

// iter sentinels
namespace ecs {
	template<typename base_T>
	struct iter_mixin<iter_sentinel<void>, base_T> {
		using sentinel = base_T;

		constexpr iter_mixin(auto* container, std::size_t idx, const auto& hint) { }
	
		template<typename Base_U>
		constexpr iter_mixin(const iter_mixin<iter_sentinel<void>, Base_U>& other) { }
	};

	// primary and entity iterator mixin
	template<typename T, typename base_T> requires(std::is_same_v<std::remove_const_t<T>, entity> || std::is_same_v<std::remove_const_t<T>, typename iter_traits<base_T>::primary_mixin>)
	struct iter_mixin<iter_sentinel<T>, base_T> {
		using sentinel = iter_sentinel<T>;

		constexpr iter_mixin(auto* container, std::size_t idx, const auto& hint)
		 : extent(container->template pool<typename iter_traits<base_T>::from_type>().size()), idx(idx) { }

		template<typename Base_U>
		constexpr iter_mixin(const iter_mixin<T, Base_U>& other) : extent(extent), idx(idx) { }
		friend constexpr bool operator==(const iter_mixin& lhs, ecs::iter_sentinel<std::remove_const_t<T>>) {
			return lhs.idx >= lhs.extent;
		}

		constexpr iter_mixin& operator++() { ++idx; return *this; }
		constexpr iter_mixin& operator--() { --idx; return *this; }

	private:
		std::size_t idx;
		std::size_t extent;
	};

	// indirect iterator mixin
	template<typename T, typename base_T> requires(std::is_same_v<std::remove_const_t<T>, indirect>)
	struct iter_mixin<iter_sentinel<T>, base_T> { 
		using sentinel = iter_sentinel<T>;

		constexpr iter_mixin(auto* container, std::size_t idx, const auto& hint)
		 : extent(container->template pool<indirect>()->size()) { }
		
		template<typename Base_U>
		constexpr iter_mixin(const iter_mixin<T, Base_U>& other) : extent(extent) { }
		
		friend constexpr bool operator==(const iter_mixin& lhs, ecs::iter_sentinel<indirect>) {
			return base_T::template get_mixin<indirect>(&lhs)->idx >= lhs.extent;
		}

		constexpr iter_mixin& operator++() { return *this; }
		constexpr iter_mixin& operator--() { return *this; }

	private:
		std::size_t extent;
	};
}