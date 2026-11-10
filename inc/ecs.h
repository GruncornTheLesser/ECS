#pragma once
#include <cstdint>
#include <compare>
#include <format>
#include <vector>
#include <memory>
#include <flat_map>

#ifndef ECS_INDEX_TYPE 
#define ECS_INDEX_TYPE uint32_t
#endif

#ifndef ECS_VERSION_TYPE 
#define ECS_VERSION_TYPE uint32_t
#endif

#ifndef ECS_INDEX_WIDTH 
#define ECS_INDEX_WIDTH 20
#endif

#ifndef ECS_VERSION_WIDTH 
#define ECS_VERSION_WIDTH 12
#endif

namespace ecs::tag {
	struct entity; 		// unique handle
	struct resource;	// single instanced class
	struct component; 	// multi instanced class, belonging to shared entity
	struct event; 		// transient class passed to emit to dispatch listener callbacks
}

namespace ecs {
	template<typename T> struct handle;
	template<typename T> struct manager;

	struct registry;
	template<typename T> struct pool;
}

namespace ecs::traits {
	template<typename T, typename D = ecs::tag::component> struct get_tag;
	template<typename T, typename D = ecs::tag::component> using get_tag_t = typename get_tag<T, D>::type;
	
	template<typename T> struct is_entity;
	template<typename T> inline constexpr bool is_entity_v = is_entity<T>::value;	
	template<typename T> concept entity_class = is_entity_v<T>;

	template<typename T> struct is_resource;
	template<typename T> inline constexpr bool is_resource_v = is_resource<T>::value;	
	template<typename T> concept resource_class = is_resource_v<T>;

	template<typename T> struct is_component;
	template<typename T> inline constexpr bool is_component_v = is_component<T>::value;	
	template<typename T> concept component_class = is_component_v<T>;

	template<typename T> struct is_event;
	template<typename T> inline constexpr bool is_event_v = is_event<T>::value;	
	template<typename T> concept event_class = is_event_v<T>;
	
	template<traits::component_class T> struct get_entity;
	template<traits::component_class T> using get_entity_t = get_entity<T>::type;
}


// sparse map
namespace ecs {
	template<typename Key_T, typename Val_T> struct sparse_map_iterator;
	template<typename Key_T, typename Val_T, typename alloc_T> struct sparse_map;

	template<typename Key_T, typename Val_T>
	struct sparse_map_iterator {
		using key_type = Key_T;
		using mapped_type = Val_T;
		using value_type = std::pair<Key_T, Val_T>;
		using reference = std::pair<const Key_T&, Val_T&>;
		using const_reference = std::pair<const Key_T&, const Val_T&>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::input_iterator_tag;
		using iterator_concept = std::random_access_iterator_tag;
		
		using key_iterator = std::vector<key_type>::const_iterator;
		using mapped_iterator = std::conditional_t<std::is_const_v<Val_T>,
			typename std::vector<std::remove_const_t<Val_T>>::const_iterator,
			typename std::vector<std::remove_const_t<Val_T>>::iterator
		>;
		struct pointer {
			reference ref;
			constexpr const reference* operator->() const noexcept;
		};
		
		constexpr sparse_map_iterator() noexcept = default;
		constexpr sparse_map_iterator(key_iterator k, mapped_iterator v) noexcept;
		constexpr sparse_map_iterator(auto& map, size_type n) noexcept;
			
		template<typename KC, typename VC> requires (std::is_const_v<Val_T> || !std::is_const_v<VC>)
		constexpr sparse_map_iterator(const KC& keys, VC& values, size_type pos) noexcept;

		// iterator -> const_iterator (public, unchanged)
		template<typename U> requires (std::is_const_v<Val_T> && !std::is_const_v<U> && std::is_same_v<Val_T, const U>)
		constexpr sparse_map_iterator(const sparse_map_iterator<Key_T, U>& other) noexcept;

		constexpr reference operator*() const noexcept;
		constexpr pointer operator->() const noexcept;
		constexpr reference operator[](difference_type n) const noexcept;
 
		constexpr sparse_map_iterator& operator++() noexcept;
		constexpr sparse_map_iterator operator++(int) noexcept;
		constexpr sparse_map_iterator& operator--() noexcept;
		constexpr sparse_map_iterator operator--(int) noexcept;
 
		constexpr sparse_map_iterator& operator+=(difference_type n) noexcept;
		constexpr sparse_map_iterator& operator-=(difference_type n) noexcept;
		constexpr sparse_map_iterator operator+(difference_type n) const noexcept;
		constexpr sparse_map_iterator operator-(difference_type n) const noexcept;
		friend constexpr sparse_map_iterator operator+(difference_type n, const sparse_map_iterator& it) { return it + n; }
 
		constexpr difference_type operator-(const sparse_map_iterator& other) const noexcept;
		constexpr auto operator<=>(const sparse_map_iterator& other) const noexcept;
		constexpr bool operator==(const sparse_map_iterator& other) const noexcept;
 
	private:
		template<typename, typename> friend struct sparse_map_iterator;
		template<typename, typename, typename> friend struct sparse_map;

		template<typename U, typename VC> requires (!std::is_const_v<Val_T> && !std::is_const_v<VC> && std::is_same_v<U, const Val_T>)
		constexpr sparse_map_iterator(const sparse_map_iterator<Key_T, U>& other, VC& values) noexcept : key_it(other.key_it) , mapped_it(values.begin() + (other.mapped_it - values.cbegin())) { }

		key_iterator key_it;
		mapped_iterator mapped_it;
	};

	template<typename Key_T, typename Val_T, typename alloc_T = std::allocator<Val_T>>
	struct sparse_map {
		static constexpr std::size_t page_size = 4096;
		using slot_type = Key_T;
		using page_type = std::unique_ptr<slot_type[]>;
		using key_type = Key_T;
		using mapped_type = Val_T;

		using page_allocator = typename std::allocator_traits<alloc_T>::template rebind_alloc<page_type>;
		using slot_allocator = typename std::allocator_traits<alloc_T>::template rebind_alloc<slot_type>;
		using key_allocator = typename std::allocator_traits<alloc_T>::template rebind_alloc<key_type>;	
		using mapped_allocator = alloc_T;
		
		using slot_container_type = std::vector<slot_type, slot_allocator>;
		using key_container_type = std::vector<key_type, key_allocator>;
		using mapped_container_type = std::vector<mapped_type, mapped_allocator>;
		
		using value_type = std::pair<Key_T, Val_T>;
		using reference = std::pair<const Key_T&, Val_T&>;
		using const_reference = std::pair<const Key_T&, const Val_T&>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		
		using iterator = sparse_map_iterator<Key_T, Val_T>;
		using const_iterator = sparse_map_iterator<Key_T, const Val_T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		
		sparse_map() noexcept = default;
		sparse_map(const sparse_map& other);
		sparse_map(sparse_map&& other) noexcept;

		template<typename alloc_U> sparse_map(const sparse_map&, const alloc_U& alloc);
		template<typename alloc_U> sparse_map(sparse_map&&, const alloc_U& alloc);
		sparse_map(key_container_type key_cont, mapped_container_type mapped_cont);
		template<typename alloc_U> sparse_map(const key_container_type& key_cont, const mapped_container_type& mapped_cont, const alloc_U& alloc);
		sparse_map(std::sorted_unique_t, key_container_type key_cont, mapped_container_type mapped_cont);
		template<typename alloc_U> sparse_map(std::sorted_unique_t, const key_container_type& key_cont, const mapped_container_type& mapped_cont, const alloc_U& alloc );
		template<typename alloc_U> explicit sparse_map(const alloc_U& alloc);
		template<typename It> sparse_map(It first, It last);
		template<typename It, typename alloc_U> sparse_map(It first, It last, const alloc_U& alloc);
		template<typename Range> sparse_map(std::from_range_t, Range&& rg);
		template<typename Range, typename alloc_U> sparse_map(std::from_range_t, Range&& rg, const alloc_U& alloc );
		template<typename It> sparse_map(std::sorted_unique_t s, It first, It last);
		template<typename It, typename alloc_U> sparse_map(std::sorted_unique_t s, It first, It last, const alloc_U& alloc);
		sparse_map(std::initializer_list<value_type> init);
		template<class alloc_U> sparse_map(std::initializer_list<value_type> init, const alloc_U& alloc );
		sparse_map(std::sorted_unique_t, std::initializer_list<value_type> init);
		template<class alloc_U> sparse_map(std::sorted_unique_t, std::initializer_list<value_type> init, const alloc_U& alloc );

		sparse_map& operator=(const sparse_map& other);
		sparse_map& operator=(sparse_map&& other);
		sparse_map& operator=(std::initializer_list<value_type> ilist);
		
		[[nodiscard]] mapped_type& at(const key_type& key);
		[[nodiscard]] const mapped_type& at(const key_type& key) const;
		
		Val_T& operator[](const key_type& key);
		Val_T& operator[](key_type&& key);

		[[nodiscard]] iterator begin() noexcept;
		[[nodiscard]] const_iterator begin() const noexcept;
		[[nodiscard]] const_iterator cbegin() const noexcept;
 		[[nodiscard]] iterator end() noexcept;
		[[nodiscard]] const_iterator end() const noexcept;
		[[nodiscard]] const_iterator cend() const noexcept;
		[[nodiscard]] reverse_iterator rbegin() noexcept;
		[[nodiscard]] const_reverse_iterator rbegin() const noexcept;
		[[nodiscard]] const_reverse_iterator crbegin() const noexcept;
		[[nodiscard]] reverse_iterator rend() noexcept;
		[[nodiscard]] const_reverse_iterator rend() const noexcept;
		[[nodiscard]] const_reverse_iterator crend() const noexcept;
		
		[[nodiscard]] size_type size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] size_type max_size() const noexcept;
		
		template<typename ... arg_Ts> std::pair<iterator, bool> emplace(arg_Ts&&... args);
		template<typename ... arg_Ts> std::pair<iterator, bool> try_emplace(key_type&& key, arg_Ts&& ... args);
		template<typename... arg_Ts> std::pair<iterator, bool> try_emplace(const key_type& key, arg_Ts&&... args);

		std::pair<iterator, bool> insert(const value_type& value);
		std::pair<iterator, bool> insert(value_type&& value);
		iterator insert(const_iterator pos, const value_type& value);
		iterator insert(const_iterator pos, value_type&& value);
		template<typename P> std::pair<iterator, bool> insert(P&& x);
		template<typename P> iterator insert(const_iterator pos, P&& x);
		template<typename It> void insert(It first, It last);
		template<typename It> void insert(std::sorted_unique_t, It first, It last);
		void insert(std::initializer_list<value_type> ilist);
		void insert(std::sorted_unique_t, std::initializer_list<value_type> ilist);
		
		template<typename Range> void insert_range(Range&& rg);

		template<class M> std::pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj);
		template<class M> std::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj);
		template<class K, class M> std::pair<iterator, bool> insert_or_assign(K&& k, M&& obj);
		template<class M> iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj);
		template<class M> iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj);
		template<class K, class M> iterator insert_or_assign(const_iterator hint, K&& k, M&& obj);

		iterator erase(iterator position);
		iterator erase(const_iterator pos);
		iterator erase(const_iterator first, const_iterator last);
		size_type erase(const key_type& key);

		void swap(sparse_map& other) noexcept;

		void clear() noexcept;

		iterator find(const key_type& key);
		const_iterator find(const key_type& key) const;
		template<class K> iterator find(const K& x);
		template<class K> const_iterator find(const K& x) const;

		bool contains(const key_type& key) const;
		template<class K> bool contains(const K& x) const;

		const key_container_type& keys() const { return dense_keys; }
		mapped_container_type& values() { return dense_values; }
		const mapped_container_type& values() const { return dense_values; }
	private:
		size_type find_pos(const key_type& key) const noexcept;
		auto slot_for(const key_type& key) -> slot_type&;
		void erase_at(size_type pos);
		void rebuild_sparse();

		static constexpr std::size_t page_shift = std::countr_zero(page_size);
		static constexpr std::size_t page_mask = page_size - 1;

		std::vector<page_type> sparse;
		std::vector<key_type> dense_keys;
		std::vector<mapped_type> dense_values;
	};
}

// handle
namespace ecs {
	template<typename T> struct handle {
		using ecs_tag = ecs::tag::entity;

		using index_type = ECS_INDEX_TYPE; 
		using version_type = ECS_VERSION_TYPE; 

		static constexpr std::size_t index_width = ECS_INDEX_WIDTH;
		static constexpr std::size_t version_width = ECS_VERSION_WIDTH;
		
		static constexpr std::size_t max_index = (static_cast<index_type>(-1u) >> (8 * sizeof(index_type) - index_width));
		
		index_type index : index_width;
		version_type version : version_width;

		friend constexpr std::strong_ordering operator<=>(const handle& lhs, const handle& rhs) { return lhs.index <=> rhs.index; }
		friend constexpr bool operator==(const handle& lhs, const handle& rhs) { return lhs.index == rhs.index && lhs.version == rhs.version; }
		operator std::size_t() const { return index; }
	};
	
	template<typename T> struct manager {
		using handle_type = T;
		using index_type = typename handle_type::index_type;
		using version_type = typename handle_type::version_type;
		
		[[nodiscard]] handle_type create();
		void destroy(handle_type hnd) noexcept;
		[[nodiscard]] bool alive(handle_type hnd) const noexcept;
		void reset() noexcept;
	private:
		std::vector<handle_type> entries;
		std::vector<index_type> free_list;
	};
	using entity = handle<void>;
}

// traits
namespace ecs::traits {
	template<typename T, typename D> struct get_tag { using type = D; };
	template<typename T, typename D> requires requires { typename T::ecs_tag; } struct get_tag<T, D> { using type = typename T::ecs_tag; };
	template<typename D> struct get_tag<void, D> : std::type_identity<void> { };
	template<typename T, typename D> using get_tag_t = typename get_tag<T, D>::type;
	
	template<typename T> struct is_resource : std::is_same<get_tag_t<std::remove_cvref_t<T>>, tag::resource> { };
	template<typename T> struct is_entity : std::is_same<get_tag_t<std::remove_cvref_t<T>>, tag::entity> { };
	template<typename T> struct is_component : std::is_same<get_tag_t<std::remove_cvref_t<T>>, tag::component> { };
	template<typename T> struct is_event : std::is_same<get_tag_t<std::remove_cvref_t<T>>, tag::event> { };
	
	template<traits::component_class T> struct get_entity { using type = ecs::entity; };
	template<traits::component_class T> requires(requires { typename T::ecs_entity; }) struct get_entity<T> { using type = T::ecs_entity; };
}

// impl
namespace ecs {
	template<traits::event_class event_T> struct listener {
		using ecs_tag = ecs::tag::component;
		using ecs_entity = handle<event_T>;
		void(*callback)(registry&, const event_T&) = nullptr;
		bool once = false;
		friend auto operator<=>(const listener& lhs, const listener& rhs) { return lhs.callback <=> rhs.callback; }
	};

	namespace event {
		template<traits::entity_class T> struct create { using ecs_tag = tag::event; T entity; };
		template<traits::entity_class T> struct destroy { using ecs_tag = tag::event; T entity; };
		
		template<traits::component_class T> struct add { using ecs_tag = tag::event; traits::get_entity_t<T> entity; T& component; };
		template<traits::component_class T> struct remove { using ecs_tag = tag::event; traits::get_entity_t<T> entity; T& component; };
	}
	
	template<typename T> struct pool { virtual ~pool() = default; };
	template<traits::resource_class T> struct pool<T> : pool<void>, T { };
	template<traits::entity_class T> struct pool<T> : pool<void>, manager<T> { };
	template<traits::component_class T> struct pool<T> : pool<void>, sparse_map<traits::get_entity_t<T>, T> { };
	template<traits::event_class T> struct pool<T> : pool<void>, manager<handle<T>>, std::flat_map<handle<T>, listener<T>> { };
}

template<>
struct std::formatter<ecs::entity> {
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	auto format(const ecs::entity& obj, std::format_context& ctx) const { return std::format_to(ctx.out(), "{}v{}", obj.index, obj.version); }
};

namespace ecs { // registry
	
	struct registry {
		using ecs_tag = void;

		// pool
		template<typename T, typename ... arg_Ts> ecs::pool<T>& get_pool(arg_Ts&&... args);
		template<typename T> const ecs::pool<T>& get_pool() const;
		template<typename T> ecs::pool<T>* try_pool();
		template<typename T> const ecs::pool<T>* try_pool() const;
		template<typename T> void remove_pool();
		
		// resource
		template<traits::resource_class T> [[nodiscard]] T& get_resource();
		template<traits::resource_class T> [[nodiscard]] const T& get_resource() const;
		template<traits::resource_class T> [[nodiscard]] T* try_resource();
		template<traits::resource_class T> [[nodiscard]] const T* try_resource() const;
		template<traits::resource_class T, typename ... arg_Ts> T& add_resource(arg_Ts&&... args);
		template<traits::resource_class T> void remove_resource();

		// entity
		template<traits::entity_class T=ecs::entity, typename ... arg_Ts> T create(arg_Ts&&...args);
		template<traits::entity_class T=ecs::entity> void destroy(T ent);
		template<traits::entity_class T=ecs::entity> bool alive(T ent) const;
		
		// event
		template<traits::event_class T> [[nodiscard]] T& get_listener(handle<T> hnd);
		template<traits::event_class T> [[nodiscard]] const T& get_listener(handle<T> hnd) const;
		template<traits::event_class T> [[nodiscard]] T* try_listener(handle<T> hnd);
		template<traits::event_class T> [[nodiscard]] const T* try_listener(handle<T> hnd) const;
		template<traits::event_class event_T> void emit(const event_T& event);
		template<typename event_T, auto func> [[nodiscard]] handle<event_T> attach(bool fire_once = false);
		template<typename event_T, typename func_T> [[nodiscard]] handle<event_T> attach(void(*callback)(registry&, const event_T&), bool fire_once = false);
		template<traits::event_class event_T> bool detach(handle<event_T> hnd);

		// component
		template<traits::component_class T> [[nodiscard]] T& get_component(traits::get_entity_t<T> hnd);
		template<traits::component_class T> [[nodiscard]] const T& get_component(traits::get_entity_t<T> hnd) const;
		template<traits::component_class T> [[nodiscard]] T* try_component(traits::get_entity_t<T> hnd);
		template<traits::component_class T> [[nodiscard]] const T* try_component(traits::get_entity_t<T> hnd) const;
		template<traits::component_class T> [[nodiscard]] bool has_component(traits::get_entity_t<T> e) const;
		template<traits::component_class T, typename ... arg_Ts> T& add_component(traits::get_entity_t<T> e, arg_Ts&& ... args);
		template<traits::component_class T> void remove_component(entity e);

		// system
		template<typename func_T, typename ... pass_Ts> decltype(auto) invoke(func_T&& func, pass_Ts&& ... args);
	private:
		inline std::size_t next_pool_index() const {
			static std::size_t next = 0;
			return next++;
		}

		template<typename T>
		inline std::size_t pool_index() const {
			static const std::size_t index = next_pool_index();
			return index;
		}
		
		std::vector<std::unique_ptr<pool<void>>> pools;
	};
}