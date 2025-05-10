#pragma once
#include <type_traits>
#include "core/meta.h"

#define ECS_ENABLE_COMPONENTS
#define ECS_ENABLE_EVENTS

#ifndef ECS_DEFAULT_TAG
#define ECS_DEFAULT_TAG ecs::tag::component
#endif

#ifndef ECS_DEFAULT_MUTEX
#define ECS_DEFAULT_MUTEX null_mutex
#endif

#ifndef ECS_DEFAULT_COMPONENT_TAG
#define ECS_DEFAULT_COMPONENT_TAG ecs::tag::component_basictype
#endif

#ifndef ECS_DEFAULT_RESOURCE_TAG
#define ECS_DEFAULT_RESOURCE_TAG ecs::tag::resource_restricted
#endif

#ifndef ECS_DEFAULT_ENTITY_TAG
#define ECS_DEFAULT_ENTITY_TAG ecs::tag::entity_dynamic
#endif

#ifndef ECS_DEFAULT_HANDLE
#define ECS_DEFAULT_HANDLE ecs::handle<uint32_t, 12>
#endif



#ifndef ECS_DEFAULT_RESOURCE_MUTEX
#define ECS_DEFAULT_RESOURCE_MUTEX null_mutex
#endif

// tags
namespace ecs::tag {
	struct resource_unrestricted { };
	struct resource_priority { };
	struct resource_restricted { };
	using resource = ECS_DEFAULT_RESOURCE_TAG; // default resource tag uses ECS_DEFAULT_RESOURCE_TAG

	struct event { };

	template<std::size_t N> struct entity_fixed { }; // fixed size entity
	struct entity_dynamic { };
	using entity = ECS_DEFAULT_ENTITY_TAG;

	struct component_basictype { };
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
	template<typename T> struct handle_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct resource_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct event_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct entity_traits;
	template<typename T, typename=traits::get_tag_t<T>> struct component_traits;

	namespace traits {
		template<typename T, typename=std::void_t<>> struct is_resource;
		template<typename T> static constexpr bool is_resource_v = is_resource<T>::value;
		template<typename T> concept resource_class = is_resource<T>::value;

		template<typename T, typename=std::void_t<>> struct is_event;
		template<typename T> static constexpr bool is_event_v = is_event<T>::value;
		template<typename T> concept event_class = is_event<T>::value;

		template<typename T, typename=std::void_t<>> struct is_entity;
		template<typename T> static constexpr bool is_entity_v = is_entity<T>::value;
		template<typename T> concept entity_class = is_entity<T>::value;
		
		template<typename T, typename=std::void_t<>> struct is_handle;
		template<typename T> static constexpr bool is_handle_v = is_handle<T>::value;
		template<typename T> concept handle_class = is_handle<T>::value;

		template<typename T, typename=std::void_t<>> struct is_component;
		template<typename T> static constexpr bool is_component_v = is_component<T>::value;
		template<typename T> concept component_class = is_component<T>::value;

		template<typename T, typename=void> struct get_traits;
		template<typename T> using get_traits_t = typename get_traits<T>::type;

		template<typename ... Ts> struct get_resource_dependencies { using type = meta::remove_if_t<meta::unique_t<meta::concat_t<typename get_traits_t<Ts>::resource_dependencies...>>, std::is_void>; };
		template<typename ... Ts> using get_resource_dependencies_t = typename get_resource_dependencies<Ts...>::type;

		template<typename ... Ts> struct get_entity_dependencies { using type = meta::unique_t<meta::remove_if_t<meta::concat_t<typename get_traits_t<Ts>::entity_dependencies...>, std::is_void>>; };
		template<typename ... Ts> using get_entity_dependencies_t = typename get_entity_dependencies<Ts...>::type;

		template<typename ... Ts> struct get_component_dependencies { using type = meta::unique_t<meta::remove_if_t<meta::concat_t<typename get_traits_t<Ts>::component_dependencies...>, std::is_void>>; };
		template<typename ... Ts> using get_component_dependencies_t = typename get_component_dependencies<Ts...>::type;
		
		template<typename ... Ts> struct get_event_dependencies { using type = meta::unique_t<meta::remove_if_t<meta::concat_t<typename get_traits_t<Ts>::event_dependencies...>, std::is_void>>; };
		template<typename ... Ts> using get_event_dependencies_t = typename get_event_dependencies<Ts...>::type;

		template<typename ... Ts> struct view_builder;

		template<typename T, typename reg_T> struct is_accessible;
		template<resource_class T, typename reg_T> struct is_accessible<T, reg_T> : meta::disjunction<typename reg_T::resource_set, std::is_same, T> { };
		template<entity_class T, typename reg_T> struct is_accessible<T, reg_T> : meta::conjunction<typename entity_traits<T>::resource_dependencies, is_accessible, reg_T> { };
		template<component_class T, typename reg_T> struct is_accessible<T, reg_T> : meta::conjunction<typename component_traits<T>::resource_dependencies, is_accessible, reg_T> { };
		template<event_class T, typename reg_T> struct is_accessible<T, reg_T> : meta::conjunction<typename event_traits<T>::resource_dependencies, is_accessible, reg_T> { };
		template<typename T, typename reg_T> static constexpr bool is_accessible_v = is_accessible<T, reg_T>::value;
	}
}

// fwd
namespace ecs {	
	template<typename Res_T, typename Mut_T> struct cache;
	template<typename ... Ts> class registry;

	// entities
	struct entity;
	template<typename T> struct event_entity;

	// handles
	template<std::unsigned_integral T, std::size_t N> struct handle;
	struct tombstone { };

	// mutexes
	enum class priority { LOW = 0, MEDIUM = 1, HIGH = 2 };
	struct null_mutex;
	struct priority_mutex;
	struct priority_shared_mutex;
	
	// resource classes
	template<typename T> struct factory;
	template<typename T> struct manager;
	template<typename T> struct indexer;
	template<typename T> struct storage;
	
	// component
	template<typename T> struct listener;

	// events
	namespace event {
		template<traits::resource_class T>  struct acquire;				// resource acquisition event
		template<traits::resource_class T>  struct release;				// resource release event

		template<traits::component_class T> struct init;   				// component initialization event
		template<traits::component_class T> struct term; 				// component termination event
		
		template<traits::entity_class T=ecs::entity> struct create; 	// handle event
		template<traits::entity_class T=ecs::entity> struct destroy;	// handle event

	}

	// services
	template<typename T, typename reg_T> class invoker;
	template<typename T, typename reg_T> class generator;
	template<typename T, typename reg_T> class pool;
	template<typename select_T, typename from_T, typename where_T, typename reg_T> class view;

	// iterators
	template<typename T, typename reg_T> struct pool_iterator;
	template<typename select_T, typename from_T, typename where_T, typename reg_T> struct view_iterator;
	struct view_sentinel;
	
	// view decorators
	template<typename ... Ts> struct select;
	template<typename T>      struct from;
	template<typename ... Ts> struct where;
	template<typename ... Ts> struct inc;
	template<typename ... Ts> struct exc;
}