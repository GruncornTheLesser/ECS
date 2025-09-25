#pragma once
#include <cstddef>
#include <concepts>

// tags
namespace ecs::tag {
	struct attribute { }; /* a variable instanced once per registry */ 
	struct entity { }; /* a table ID, with which multiple components can associate */ 
	struct component { }; /* a data point which can dynamically be associate with an entity */ 
	struct flag { }; /* a valueless and non iterable component */
	struct event { }; /* a callback that executes attached listeners */
	struct resource { }; /* a data point with a built handle management. */

	// TODO:
	template<typename ... Ts> struct archetype : component { };
	template<typename ... Ts> struct uniontype : component { };
}

namespace ecs::policy {
	// execution policy
	struct immediate { }; // executes operation immediately.
	struct deferred { }; // defers operation until sync, does not modify primary structure.
	// ??? struct lazy { }; // reorders primary structures but waits until sync to call events or modify storage.

	// sequence policy
	struct optimal { }; // uses swap and insert or swap and pop to insert/erase component. 
	struct strict { }; // maintains the order of components.
	// ??? struct grouped { }; // maintains the order of components.
	// ??? struct sorted { }; // maintains the order of components.
}

namespace ecs::traits {
	template<typename T> struct is_attribute;
	template<typename T> static constexpr bool is_attribute_v = is_attribute<T>::value;
	template<typename T> concept attribute_class = is_attribute<T>::value;

	template<typename T> struct is_entity;
	template<typename T> static constexpr bool is_entity_v = is_entity<T>::value;
	template<typename T> concept entity_class = is_entity<T>::value;

	template<typename T> struct is_component;
	template<typename T> static constexpr bool is_component_v = is_component<T>::value;
	template<typename T> concept component_class = is_component<T>::value;

	template<typename T> struct is_event;
	template<typename T> static constexpr bool is_event_v = is_event<T>::value;
	template<typename T> concept event_class = is_event<T>::value;

	//? template<typename T> struct is_resource;
	//? template<typename T> static constexpr bool is_resource_v = is_resource<T>::value;
	//? template<typename T> concept resource_class = is_resource<T>::value;

	template<typename, typename=void, typename=void> struct view_builder;
}

// fwd
namespace ecs {
	template<typename ... Ts> class registry;
	
	// handle primitive
	struct tombstone { };
	template<std::unsigned_integral T, std::size_t N> struct handle;
	
	// mutex primitives
	enum class priority { LOW = 0, MEDIUM = 1, HIGH = 2 };
	struct priority_mutex;
	struct priority_shared_mutex;

	// entities
	struct entity;

	// attributes
	template<ecs::traits::entity_class T>	 struct factory;
	template<ecs::traits::entity_class T>	 struct archive;
	template<ecs::traits::component_class T> struct manager;
	template<ecs::traits::component_class T> struct indexer;
	template<ecs::traits::component_class T> struct storage;


	// components
	// ? template<ecs::traits::event_class T> struct listener;

	// events
	namespace event {		
		template<traits::entity_class T> struct create;
		template<traits::entity_class T> struct destroy;
		template<traits::component_class T> struct initialize;
		template<traits::component_class T> struct terminate;
		template<traits::attribute_class T> struct acquire;
		template<traits::attribute_class T> struct release;
	}

	// services
	template<ecs::traits::event_class T, typename reg_T> class invoker;
	template<ecs::traits::entity_class T, typename reg_T> class generator;
	template<ecs::traits::component_class T, typename reg_T> class pool;
	template<typename select_T, typename from_T, typename where_T, typename reg_T> class view;
	template<typename lock_T, typename reg_T> class pipeline;

	// iterators
	template<typename select_T, typename from_T, typename where_T, typename reg_T> struct view_iterator;
	struct view_sentinel;
	
	// view decorators
	template<traits::component_class ... Ts> struct inc;
	template<traits::component_class ... Ts> struct exc;
	// ? template<traits::component_class T> struct cnd;

	template<typename ... Ts> struct select { };
	template<typename T> struct from { };
	template<typename ... Ts> struct where { };
}