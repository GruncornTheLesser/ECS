#pragma once
#include <cstddef>
#include <concepts>

// tags
namespace ecs::tag {
	struct resource { };
	struct entity { };
	struct component { };
	struct event { };
	
	namespace policy {
		struct exec { };
		struct seq { };
	};

	// special resource
	template<typename ... Ts> struct resource_set { };

	// special component
	template<typename T, typename ... Ts> struct archetype { };
	template<typename T, typename ... Ts> struct uniontype { };
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

	template<typename, typename=void, typename=void> struct view_builder;

	template<typename reg_T, typename T> struct is_accessible;
	template<typename reg_T, typename T> static constexpr bool is_accessible_v = is_accessible<reg_T, T>::value;
}

// fwd
namespace ecs {
	template<ecs::traits::resource_class res_T> struct cache;
	template<typename ... Ts> class registry; // Ts=event or entity or component or resource
	
	// handle primitive
	struct tombstone { };
	template<std::unsigned_integral T, std::size_t N> struct handle;
	
	// mutex primitives
	enum class priority { LOW = 0, MEDIUM = 1, HIGH = 2 };
	struct null_mutex;
	struct priority_mutex;
	struct priority_shared_mutex;

	// entities
	struct entity;
	template<ecs::traits::event_class T> struct event_entity;

	// resource classes
	template<ecs::traits::entity_class ent_T>     struct factory;
	template<ecs::traits::component_class comp_T> struct manager;
	template<ecs::traits::component_class comp_T> struct indexer;
	template<ecs::traits::component_class comp_T> struct storage;

	
	// component
	template<ecs::traits::event_class T> struct listener;
	template<ecs::traits::event_class T> struct once_tag;

	// events
	namespace event {		
		template<traits::component_class T> struct initialize;
		template<traits::component_class T> struct terminate;
		
		template<traits::entity_class T> struct create;
		template<traits::entity_class T> struct destroy;
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
	template<traits::component_class ... Ts> struct inc;
	template<traits::component_class ... Ts> struct exc;
	template<traits::component_class T>		 struct cnd;
}