#pragma once
#include <stdint.h>
#include <concepts>

#ifndef ECS_DEFAULT_TAG
#define ECS_DEFAULT_TAG ecs::tag::component
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

#ifndef ECS_DEFAULT_EVENT_TAG
#define ECS_DEFAULT_EVENT_TAG ecs::tag::synced_event
#endif

#ifndef ECS_DEFAULT_HANDLE_INTEGRAL
#define ECS_DEFAULT_HANDLE_INTEGRAL uint32_t
#endif

#ifndef ECS_DEFAULT_HANDLE_VERSION_WIDTH
#define ECS_DEFAULT_HANDLE_VERSION_WIDTH 12
#endif

#ifndef ECS_DEFAULT_RESOURCE_MUTEX
#define ECS_DEFAULT_RESOURCE_MUTEX null_mutex
#endif

#ifndef ECS_DEFAULT_INITIALIZE_EVENT
#define ECS_DEFAULT_INITIALIZE_EVENT ecs::event::init<T>
#endif

#ifndef ECS_DEFAULT_TERMINATE_EVENT
#define ECS_DEFAULT_TERMINATE_EVENT ecs::event::term<T>
#endif

#ifndef ECS_DEFAULT_PAGE_SIZE
#define ECS_DEFAULT_PAGE_SIZE 4096
#endif

// standalone trait value macro declaration
#define EXPAND(...) __VA_ARGS__

#define DYNAMIC_TRAIT_TYPE(NAME, DEFAULT) 										\
template<typename T, typename D=DEFAULT> 										\
struct get_##NAME { using type = D; }; 											\
template<typename T, typename D> requires requires { typename T::NAME##_type; }	\
struct get_##NAME<T, D> { using type = typename T::NAME##_type; };	            \
template<typename T, typename D=DEFAULT>										\
using get_##NAME##_t = typename get_##NAME<T, D>::type;

#define STATIC_TRAIT_TYPE(NAME, TRAIT)											\
template<typename T>															\
struct get_##NAME { using type = typename TRAIT##_traits<T>::NAME##_type; };	\
template<typename T>															\
using get_##NAME##_t = typename get_##NAME<T>::type;


// standalone trait type macro declaration
#define DYNAMIC_TRAIT_VALUE(TYPE, NAME, DEFAULT) 								\
template<typename T, typename=void> 											\
struct get_##NAME { static constexpr TYPE value = DEFAULT; };		 			\
template<typename T, typename D> requires requires { T::Name; }					\
struct get_##NAME<T, D> { static constexpr TYPE value = T::NAME; };				\
template<typename T>															\
static constexpr TYPE get_##NAME##_v = get_##NAME<T>::value;

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
	DYNAMIC_TRAIT_TYPE(ecs_tag, ECS_DEFAULT_TAG)
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

	template<typename ... Ts> struct view_builder;

	template<typename T, typename reg_T> struct is_accessible;	
	template<typename T, typename reg_T> static constexpr bool is_accessible_v = is_accessible<T, reg_T>::value;
}

// fwd
namespace ecs {	
	template<ecs::traits::resource_class Res_T, typename Mut_T> struct cache;
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
	template<ecs::traits::event_class T> struct listener;

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
	template<ecs::traits::component_class T, typename reg_T> struct pool_iterator;
	template<typename select_T, typename from_T, typename where_T, typename reg_T> struct view_iterator;
	struct view_sentinel;
	
	// view decorators
	template<typename ... Ts> struct select;
	template<typename T>      struct from;
	template<typename ... Ts> struct where;
	template<ecs::traits::component_class ... Ts> struct inc;
	template<ecs::traits::component_class ... Ts> struct exc;
}