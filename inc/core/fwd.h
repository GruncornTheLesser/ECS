#pragma once
#include <cstddef>
#include <concepts>

// tags
namespace ecs::tag {
	struct attribute { }; /* singletons instanced once per registry */ 
	struct entity { }; /* a table ID, with which multiple components can associate */ 
	struct component { }; /* a data point which can dynamically associate with an entity */ 

	// special component
	/* a valueless and non iterable component */
	struct flag { };
	/* a data point unassociated with an entity type */
	struct unit { };
	/* a callback that executes attached listeners */
	struct event { };
	/* a component */
	struct resource { };

	template<typename ... Ts> struct archetype;
	template<> struct archetype<> { };
	template<typename ... Ts> struct archetype : archetype<> { };

	template<typename ... Ts> struct uniontype;
	template<> struct uniontype<> { };
	template<typename ... Ts> struct uniontype : uniontype<> { };
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

	template<typename, typename=void, typename=void> struct view_builder;
}

// fwd
namespace ecs {
	template<typename ... Ts> class registry;
	
	// handle primitive
	struct tombstone { };
	template<std::unsigned_integral T, std::size_t N> struct index;
	template<std::unsigned_integral T, std::size_t N> struct version;
	template<std::unsigned_integral T, std::size_t N> struct handle;
	
	// mutex primitives
	enum class priority { LOW = 0, MEDIUM = 1, HIGH = 2 };
	struct priority_mutex;
	struct priority_shared_mutex;

	// entities
	struct entity;
	template<ecs::traits::event_class T> struct event_entity;

	// attributes
	template<ecs::traits::entity_class ent_T>     struct factory;
	template<ecs::traits::entity_class ent_T>     struct archive;
	template<ecs::traits::component_class comp_T> struct manager;
	template<ecs::traits::component_class comp_T> struct indexer;
	template<ecs::traits::component_class T>      struct storage;
	
	// components
	template<ecs::traits::event_class T> struct listener;

	// events
	namespace event {		
		template<traits::entity_class T> struct create;
		template<traits::entity_class T> struct destroy;
		template<traits::component_class T> struct initialize;
		template<traits::component_class T> struct terminate;
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
	template<typename ... Ts> struct select { };
	template<traits::component_class T> struct from { };
	template<typename ... Ts> struct where { };

	template<traits::component_class ... Ts> struct inc;
	template<traits::component_class ... Ts> struct exc;
	template<traits::component_class T>		 struct cnd;

	template<typename T, typename page_T, typename alloc_T> struct packed;
	template<typename T, typename page_T, typename alloc_T> struct sparse;

}