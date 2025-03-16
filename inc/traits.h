#pragma once
#include <utility>
#include <type_traits>
#include <shared_mutex>
#include <functional>
#include "meta.h"

// default tag macros
#ifndef ECS_DEFAULT_TAG
#define ECS_DEFAULT_TAG ecs::tag::component
#endif

#ifndef ECS_DEFAULT_COMPONENT_TAG
#define ECS_DEFAULT_COMPONENT_TAG ecs::tag::component_basictype<ecs::table>
#endif

#ifndef ECS_DEFAULT_EVENT_TAG
#define ECS_DEFAULT_EVENT_TAG ecs::tag::event_sync
#endif

#ifndef ECS_DEFAULT_RESOURCE_TAG
#define ECS_DEFAULT_RESOURCE_TAG ecs::tag::resource_restricted
#endif

#ifndef ECS_DEFAULT_TABLE_TAG
#define ECS_DEFAULT_TABLE_TAG ecs::tag::table_dynamic<std::allocator<void>>
#endif

#ifndef ECS_DEFAULT_HANDLE
#define ECS_DEFAULT_HANDLE ecs::handle<uint32_t, 12>
#endif

/*
? maybe use table as service to replace generator
? generator then implements more details, eg destroy components when destroying entity

*/


// fwd 
namespace ecs {
	struct table;
}

// tags
namespace ecs::tag {
	struct handle { };

	struct resource_custom { }; // must define 
	struct resource_unrestricted { };
	struct resource_priority { };
	struct resource_restricted { };
	struct resource_exclusive_priority { };
	struct resource_exclusive_restricted { };
	using resource = ECS_DEFAULT_RESOURCE_TAG; // default resource tag uses ECS_DEFAULT_RESOURCE_TAG

	struct event_custom { }; // must define: handle_type, invoker_type, listener_type, bool enable_async, bool strict_order
	struct event_sync { };
	struct event_async { };
	using event = ECS_DEFAULT_EVENT_TAG;  // default event tag uses ECS_DEFAULT_EVENT_TAG

	struct table_custom { }; // must define: allocator_type, factory_type, handle_type
	template<std::size_t N> struct table_fixed { }; // fixed size table
	template<typename Alloc_T> struct table_dynamic { };
	using table = ECS_DEFAULT_TABLE_TAG;

	struct component_custom { }; // must define: table_type, manager_type, indexer_type, storage_type
	template<typename T> struct component_basictype { };
	template<typename T, typename ... Ts> struct component_archetype { };
	template<typename T, typename ... Ts> struct component_uniontype { };
	using component = ECS_DEFAULT_COMPONENT_TAG;   // default component tag uses ECS_DEFAULT_COMPONENT_TAG
}

// traits
namespace ecs {
	namespace traits {
		template<typename T, typename=std::void_t<>> struct get_tag { using type = ECS_DEFAULT_TAG; };
		template<typename T> struct get_tag<T, std::void_t<typename T::ecs_tag>> { using type = typename T::ecs_tag; };
		template<typename T> using get_tag_t = typename get_tag<T>::type;
	}

	// traits
	template<typename T, typename=traits::get_tag_t<T>> struct resource_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct event_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct table_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct handle_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct component_traits;

	namespace traits {
		template<typename T, typename=std::void_t<>> struct is_resource;
		template<typename T> static constexpr bool is_resource_v = is_resource<T>::value;
		template<typename T> concept resource_class = is_resource<T>::value;

		template<typename T, typename=std::void_t<>> struct is_event;
		template<typename T> static constexpr bool is_event_v = is_event<T>::value;
		template<typename T> concept event_class = is_event<T>::value;
		
		template<typename T, typename=std::void_t<>> struct is_table;
		template<typename T> static constexpr bool is_table_v = is_table<T>::value;
		template<typename T> concept table_class = is_table<T>::value;
		
		template<typename T, typename=std::void_t<>> struct is_handle;
		template<typename T> static constexpr bool is_handle_v = is_handle<T>::value;
		template<typename T> concept handle_class = is_handle<T>::value;

		template<typename T, typename=std::void_t<>> struct is_component;
		template<typename T> static constexpr bool is_component_v = is_component<T>::value;
		template<typename T> concept component_class = is_component<T>::value;

		template<typename T, typename=void> struct get_traits;
		template<typename T> using get_traits_t = typename get_traits<T>::type;

		template<typename T> struct get_resource_dependencies { using type = typename get_traits_t<T>::resource_dependencies; };
		template<typename T> using get_resource_dependencies_t = typename get_resource_dependencies<T>::type;

		template<typename T> struct get_table_dependencies { using type = typename get_traits_t<T>::table_dependencies; };
		template<typename T> using get_table_dependencies_t = typename get_table_dependencies<T>::type;
		
		template<typename T> struct get_component_dependencies { using type = typename get_traits_t<T>::component_dependencies; };
		template<typename T> using get_component_dependencies_t = typename get_component_dependencies<T>::type;

		template<typename T> struct get_event_dependencies { using type = typename get_traits_t<T>::event_dependencies; };
		template<typename T> using get_event_dependencies_t = typename get_event_dependencies<T>::type;

		template<typename ... Ts> class registry_builder;
		template<typename ... Ts> struct pipeline_builder;
		template<typename ... Ts> struct view_builder;

		template<typename reg_T, typename T> struct is_compatible;
		template<typename ... Ts, typename T> struct is_compatible<std::tuple<Ts...>, T> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> { };
		template<typename reg_T, table_class T> struct is_compatible<reg_T, T> : is_compatible<typename reg_T::handle_set, std::remove_const_t<T>> { };
		template<typename reg_T, event_class T> struct is_compatible<reg_T, T> : is_compatible<typename reg_T::event_set, std::remove_const_t<T>> { };
		template<typename reg_T, component_class T> struct is_compatible<reg_T, T> : is_compatible<typename reg_T::component_set, std::remove_const_t<T>> { };
		template<typename reg_T, resource_class T> struct is_compatible<reg_T, T> : is_compatible<typename reg_T::resource_set, std::remove_const_t<T>> { };
		template<typename reg_T, typename T> static constexpr bool is_compatible_v = is_compatible<reg_T, T>::value;

		// event_traits

		template<typename T> struct get_invoker { using type = typename event_traits<std::remove_const_t<T>>::invoker_type; };
		template<typename T> using get_invoker_t = typename get_invoker<T>::type;
		
		// table_traits

		template<typename T> struct get_handle { using type = typename table_traits<std::remove_const_t<T>>::handle_type; };
		template<typename T> using get_handle_t = typename get_handle<T>::type;
		template<typename T> struct get_factory { using type = typename table_traits<std::remove_const_t<T>>::factory_type; };
		template<typename T> using get_factory_t = typename get_factory<T>::type;
		
		// handle_traits

		template<typename T> struct get_value { using type = typename handle_traits<std::remove_const_t<T>>::value_type; };
		template<typename T> using get_value_t = typename get_value<T>::type;  
		template<typename T> struct get_version { using type = typename handle_traits<std::remove_const_t<T>>::version_type; };
		template<typename T> using get_version_t = typename get_version<T>::type;  
		template<typename T> struct get_integral { using type = typename handle_traits<std::remove_const_t<T>>::integral_type; };
		template<typename T> using get_integral_t = typename get_integral<T>::type;  

		// component_traits

		template<typename T> struct get_table { using type = typename component_traits<std::remove_const_t<T>>::table_type; };
		template<typename T> using get_table_t = typename get_table<T>::type;
		template<typename T> struct get_manager { using type = typename component_traits<std::remove_const_t<T>>::manager_type; };
		template<typename T> using get_manager_t = typename get_manager<T>::type;
		template<typename T> struct get_indexer { using type = typename component_traits<std::remove_const_t<T>>::indexer_type; };
		template<typename T> using get_indexer_t = typename get_indexer<T>::type;
		template<typename T> struct get_storage { using type = typename component_traits<std::remove_const_t<T>>::storage_type; };
		template<typename T> using get_storage_t = typename get_storage<T>::type;
	}
}

// fwd
namespace ecs {
	// container
	template<typename ... Ts> class registry;

	enum class priority { LOW = 0, MEDIUM = 1, HIGH = 2 };
	struct priority_mutex;
	struct priority_shared_mutex;
	
	// default resource types -> implements the behaiour defined in the traits classes
	template<typename T> struct factory;
	template<typename T> struct manager;
	template<typename T> struct indexer;
	template<typename T> struct storage;
	template<typename T> struct invoker;
	
	// table
	struct table { using ecs_tag = ecs::tag::table; }; // default table
	
	// events
	namespace event {
		template<traits::resource_class T>  struct acquire;   // resource event
		template<traits::resource_class T>  struct release;   // resource event
		template<traits::component_class T> struct init;      // component event
		template<traits::component_class T> struct terminate; // component event
		template<traits::table_class T=ecs::table> struct create;    // handle event
		template<traits::table_class T=ecs::table> struct destroy;   // handle event
	}
	// handles
	struct entity;
	template<std::unsigned_integral T, std::size_t N> struct handle;
	struct tombstone { };

	// services
	//template<typename set_T, typename reg_T> class pipeline;
	template<typename T, typename reg_T> class generator;
	template<typename T, typename reg_T> class pool; // TODO: maybe include policy here??
	template<typename select_T, typename from_T, typename where_T, typename reg_T> class view;

	// iterators
	template<typename T, typename reg_T> struct pool_iterator;
	template<typename select_T, typename from_T, typename where_T, typename reg_T> struct view_iterator;
	struct view_sentinel;
	
	template<typename ... Ts> struct select;
	template<typename T>      struct from;
	template<typename ... Ts> struct where;
	// where elements
	template<typename ... Ts> struct include;
	template<typename ... Ts> struct exclude;
}

// resource traits
namespace ecs {
	template<typename T>
	struct resource_traits<T, ecs::tag::resource_custom>
	{
		using container_type = typename T::container; // a wrapper around the resource holding the meta data of the resource acquisition
		
		static inline constexpr void release(container_type& cont) { cont.release(); }
		static inline constexpr void release(const container_type& cont) { cont.release(); }
		static inline constexpr void acquire(container_type& cont, priority p) { cont.acquire(p); }
		static inline constexpr void acquire(const container_type& cont, priority p) { cont.acquire(p); }

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_unrestricted>
	{
		using container_type = T;

		static inline constexpr void release(container_type& cont) { }
		static inline constexpr void release(const container_type& cont) { }
		static inline constexpr void acquire(container_type& cont, priority p) { }
		static inline constexpr void acquire(const container_type& cont, priority p) { }

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_priority>
	{
		using container_type = std::pair<T, priority_shared_mutex>;
		
		static inline constexpr void release(container_type& cont) { std::get<1>(cont).unlock(); }
		static inline constexpr void release(const container_type& cont) { std::get<1>(cont).unlock(); }
		static inline constexpr void acquire(container_type& cont, priority p) { std::get<1>(cont).lock(p); }
		static inline constexpr void acquire(const container_type& cont, priority p) { std::get<1>(cont).lock(p); }

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_restricted>
	{
		using container_type = std::pair<T, std::shared_mutex>;
		using identifier_type = T;
		
		static inline constexpr void release(container_type& cont) { std::get<1>(cont).unlock(); }
		static inline constexpr void release(const container_type& cont) { std::get<1>(cont).unlock(); }
		static inline constexpr void acquire(container_type& cont, priority p) { std::get<1>(cont).lock(); }
		static inline constexpr void acquire(const container_type& cont, priority p) { std::get<1>(cont).lock(); }

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_exclusive_priority>
	{
		using container_type = typename std::pair<T, priority_mutex>;
		using identifier_type = T;
		
		static inline constexpr void release(container_type& cont) { std::get<1>(cont).unlock(); }
		static inline constexpr void release(const container_type& cont) { std::get<1>(cont).unlock(); }
		static inline constexpr void acquire(container_type& cont, priority p) { std::get<1>(cont).lock(p); }
		static inline constexpr void acquire(const container_type& cont, priority p) { std::get<1>(cont).lock(p); }

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};
	
	template<typename T>
	struct resource_traits<T, ecs::tag::resource_exclusive_restricted>
	{
		using container_type = typename T::resource_container;
		using identifier_type = T;
		
		static inline constexpr void release(container_type& res) { res.release(); }
		static inline constexpr void release(const container_type& res) { res.release(); }
		static inline constexpr void acquire(container_type& res, priority p) { res.acquire(p); }
		static inline constexpr void acquire(const container_type& res, priority p) { res.acquire(p); }

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};
}

// event traits
namespace ecs {
	template<typename T>
	struct event_traits<T, ecs::tag::event_custom>
	{
		using handle_type = typename T::handle_type;
		using invoker_type = typename T::invoker_type;
		using listener_type = typename T::listener_type;
		static constexpr bool enable_async = T::enable_async;
		static constexpr bool strict_order = T::strict_order;

		using resource_dependencies = std::tuple<invoker_type>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<T>;
	};

	template<typename T>
	struct event_traits<T, ecs::tag::event_sync> {
		using handle_type = uint32_t;
		using invoker_type = invoker<T>;
		using listener_type = std::function<void(T&)>;
		static constexpr bool enable_async = false;
		static constexpr bool strict_order = true;

		using resource_dependencies = std::tuple<invoker_type>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<T>;
	};

	template<typename T>
	struct event_traits<T, ecs::tag::event_async> { 
		using handle_type = uint32_t;
		using invoker_type = invoker<T>;
		using listener_type = std::function<void(T&)>;
		static constexpr bool enable_async = true;
		static constexpr bool strict_order = false;

		using resource_dependencies = std::tuple<invoker_type>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<T>;
	};
}

// handle traits
namespace ecs {
	template<typename T>
	struct handle_traits<T, ecs::tag::handle> {
		using value_type = typename T::value_type;
		using version_type = typename T::version_type;
		using integral_type = typename T::integral_type;
	};

	template<>
	struct handle_traits<entity, ecs::tag::handle> {
		using value_type = void;
		using version_type = void;
		using integral_type = size_t;
	};
}

// table traits
namespace ecs {
	template<typename T>
	struct table_traits<T, ecs::tag::table_custom> {
		using allocator_type = typename T::allocator_type;
		using factory_type = typename T::factory_type;
		using handle_type = typename T::handle_type;

		using resource_dependencies = std::tuple<factory_type>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<T>;
		using event_dependencies = std::tuple<>;
		
	};

	template<typename T, typename alloc_T>
	struct table_traits<T, ecs::tag::table_dynamic<alloc_T>> {
		using allocator_type = alloc_T;
		using handle_type = ECS_DEFAULT_HANDLE;
		using factory_type = factory<T>;

		using resource_dependencies = std::tuple<factory_type>;
		using component_dependencies = std::tuple<>;
		using table_dependencies = std::tuple<T>;
		using event_dependencies = std::tuple<>;
	};
	
	template<typename T, std::size_t N>
	struct table_traits<T, ecs::tag::table_fixed<N>> { 
		using allocator_type = void; // TODO: registry resource shared allocator
		using handle_type = handle<uint32_t, 12>;
		using factory_type = factory<T>;
	};
}

// component traits
namespace ecs {
	template<typename T>
	struct component_traits<T, tag::component_custom>
	{
		using table_type = typename T::table_type;
		using manager_type = typename T::manager_type;
		using indexer_type = typename T::indexer_type;
		using storage_type = typename T::storage_type;

		static_assert(traits::is_table_v<table_type>);
		static_assert(traits::is_resource_v<manager_type>);
		static_assert(traits::is_resource_v<indexer_type>);
		static_assert(traits::is_resource_v<storage_type>);

		using resource_dependencies = meta::concat_t<std::tuple<manager_type, indexer_type, storage_type>, traits::get_resource_dependencies_t<table_type>>;
		using component_dependencies = meta::concat_t<std::tuple<T>, traits::get_component_dependencies_t<table_type>>;
		using table_dependencies = meta::concat_t<std::tuple<table_type>, traits::get_table_dependencies_t<table_type>>;
		using event_dependencies = traits::get_event_dependencies_t<table_type>;
	};

	template<typename T, typename table_T>
	struct component_traits<T, tag::component_basictype<table_T>>
	{
		using table_type = table_T;
		using manager_type = manager<T>;
		using indexer_type = indexer<T>;
		using storage_type = storage<T>;

		using resource_dependencies = std::tuple<traits::get_factory_t<table_type>, manager_type, indexer_type, storage_type>;
		using component_dependencies = std::tuple<T>;
		using table_dependencies = std::tuple<table_type>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T, typename table_T, typename U, typename ... Ts>
	struct component_traits<T, tag::component_archetype<table_T, U, Ts...>>
	{
		using table_type = table_T;
		using manager_type = manager<U>;
		using indexer_type = indexer<U>;
		using storage_type = storage<T>;

		using resource_dependencies = std::tuple<traits::get_factory_t<table_type>, manager_type, indexer_type, storage_type, traits::get_storage_t<Ts>...>;
		using component_dependencies = std::tuple<U, Ts...>;
		using table_dependencies = std::tuple<table_type>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T, typename table_T, typename U, typename ... Ts>
	struct component_traits<T, tag::component_uniontype<table_T, U, Ts...>>
	{
		using table_type = table_T;
		using manager_type = manager<U>;
		using indexer_type = indexer<U>;
		using storage_type = storage<meta::sort_by_t<std::variant<U, Ts...>>>;

		using resource_dependencies = std::tuple<traits::get_factory_t<table_type>, manager_type, indexer_type, storage_type>;
		using component_dependencies = std::tuple<U, Ts...>;
		using table_dependencies = std::tuple<table_type>;
		using event_dependencies = std::tuple<>;
	};
}

// concepts definitions
namespace ecs::traits {
	template<typename T, typename> struct is_resource : std::false_type { };
	template<typename T> struct is_resource<T, std::void_t<decltype(resource_traits<std::remove_const_t<T>>{})>> : std::true_type { };

	template<typename T, typename> struct is_event : std::false_type { };
	template<typename T> struct is_event<T, std::void_t<decltype(event_traits<std::remove_const_t<T>>{})>> : std::true_type { };
	
	template<typename T, typename> struct is_table : std::false_type { };
	template<typename T> struct is_table<T, std::void_t<decltype(table_traits<std::remove_const_t<T>>{})>> : std::true_type { };

	template<typename T, typename> struct is_handle : std::false_type { };
	template<typename T> struct is_handle<T, std::void_t<decltype(handle_traits<std::remove_const_t<T>>{})>> : std::true_type { };

	template<typename T, typename> struct is_component : std::false_type { };
	template<typename T> struct is_component<T, std::void_t<decltype(component_traits<std::remove_const_t<T>>{})>> : std::true_type { };
	
	template<typename T> struct get_traits<T, std::enable_if_t<is_resource_v<T>>> { using type = resource_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_event_v<T>>> { using type = event_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_table_v<T>>> { using type = table_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_handle_v<T>>> { using type = handle_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_component_v<T>>> { using type = component_traits<T>; };
}