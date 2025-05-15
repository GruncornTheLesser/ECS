#pragma once
#include <cstddef>
#include <concepts>

// tags
namespace ecs::tag {
	struct resource { };
	struct resource_unrestricted : resource { };
	struct resource_priority : resource { };
	struct resource_restricted : resource { };

	struct entity { };
	struct entity_dynamic : entity { };
	template<std::size_t N> struct entity_fixed : entity { }; // fixed size entity
	struct unversioned_entity : entity { };
	
	struct component { };
	struct component_basictype : component { };
	template<typename T, typename ... Ts> struct component_archetype : component { };
	template<typename T, typename ... Ts> struct component_uniontype : component { };
	
	struct event { };
	struct synced_event : event { };
}

namespace ecs::traits {
	template<typename T> struct is_resource;
	template<typename T> static constexpr bool is_resource_v = is_resource<T>::value;
	template<typename T> concept resource_class = is_resource<T>::value;

	template<typename T> struct is_event;
	template<typename T> static constexpr bool is_event_v = is_event<T>::value;
	template<typename T> concept event_class = is_event<T>::value;

	template<typename T> struct is_entity;
	template<typename T> static constexpr bool is_entity_v = is_entity<T>::value;
	template<typename T> concept entity_class = is_entity<T>::value;

	template<typename T> struct is_component;
	template<typename T> static constexpr bool is_component_v = is_component<T>::value;
	template<typename T> concept component_class = is_component<T>::value;

	template<typename T, typename=void> struct get_traits;
	template<typename T> using get_traits_t = typename get_traits<T>::type;

	template<typename ... Ts> struct get_resource_dependencies;
	template<typename ... Ts> using get_resource_dependencies_t = typename get_resource_dependencies<Ts...>::type;

	template<typename ... Ts> struct get_entity_dependencies;
	template<typename ... Ts> using get_entity_dependencies_t = typename get_entity_dependencies<Ts...>::type;

	template<typename ... Ts> struct get_component_dependencies;
	template<typename ... Ts> using get_component_dependencies_t = typename get_component_dependencies<Ts...>::type;

	template<typename, typename=void, typename=void> struct view_builder;

	template<typename T> struct is_data_component;
	template<typename T> static constexpr bool is_empty_component_v = is_data_component<T>::value;


	template<typename T, typename manager_T> struct is_manager_match;
	template<typename T, typename manager_T> static constexpr bool is_manager_match_v = is_manager_match<T, manager_T>::value;

	template<typename T, typename indexer_T> struct is_indexer_match;
	template<typename T, typename indexer_T> static constexpr bool is_indexer_match_v = is_indexer_match<T, indexer_T>::value;

	template<typename T, typename storage_T> struct is_storage_match;
	template<typename T, typename storage_T> static constexpr bool is_storage_match_v = is_storage_match<T, storage_T>::value;

	template<typename T, typename entity_T>  struct is_entity_match;
	template<typename T, typename entity_T>  static constexpr bool is_entity_match_v = is_entity_match<T, entity_T>::value;

	template<typename T, typename U> struct is_const_accessible;
	template<typename T, typename U> static constexpr bool is_const_accessible_v = is_const_accessible<T, U>::value;

	template<typename T, typename reg_T> struct is_accessible;	
	template<typename T, typename reg_T> static constexpr bool is_accessible_v = is_accessible<T, reg_T>::value;
}

// fwd
namespace ecs {	
	template<ecs::traits::resource_class Res_T> struct cache;
	template<typename ... Ts> class registry;

	// entities
	struct entity;
	template<ecs::traits::event_class T> struct event_entity;

	// handles
	struct tombstone { };
	template<std::unsigned_integral T, std::size_t N> struct handle;

	// mutexes
	enum class priority { LOW = 0, MEDIUM = 1, HIGH = 2 };
	struct null_mutex;
	struct priority_mutex;
	struct priority_shared_mutex;
	
	// resource classes
	template<ecs::traits::entity_class T>    struct factory;
	template<ecs::traits::component_class T> struct manager;
	template<ecs::traits::component_class T> struct indexer;
	template<ecs::traits::component_class T> struct storage;
	
	// component
	template<ecs::traits::event_class T, typename callback_T> struct listener;

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
	template<ecs::traits::event_class T, typename reg_T> class invoker;
	template<ecs::traits::entity_class T, typename reg_T> class generator;
	template<ecs::traits::component_class T, typename reg_T> class pool;
	template<typename select_T, typename from_T, typename where_T, typename reg_T> class view;

	// iterators
	template<typename select_T, typename from_T, typename where_T, typename reg_T> struct view_iterator;
	struct view_sentinel;
	
	// view decorators
	template<typename ... Ts> requires ((traits::is_entity_v<Ts> || traits::is_component_v<Ts>) && ...) struct select { };
	template<traits::component_class T> struct from { };
	template<typename ... Ts> struct where { };
	template<ecs::traits::component_class ... Ts> struct inc;
	template<ecs::traits::component_class ... Ts> struct exc;
}