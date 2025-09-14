#pragma once

#include "core/fwd.h"
#include <util.h>
#include <cstdint>

#ifndef ECS_RECURSIVE_DEPENDENCY
#define ECS_RECURSIVE_DEPENDENCY true
#endif

#ifndef ECS_DYNAMIC_REGISTRY
#define ECS_DYNAMIC_REGISTRY true
#endif

/** the default type tag used if struct does not declare ecs_category */
#ifndef ECS_DEFAULT_TAG
#define ECS_DEFAULT_TAG ecs::tag::component
#endif

/* the default integral used if entity does not declared handle_integral_type  */
#ifndef ECS_DEFAULT_HANDLE
#define ECS_DEFAULT_HANDLE ecs::handle<uint32_t, 12>
#endif

#ifndef ECS_DEFAULT_PAGE_SIZE
#define ECS_DEFAULT_PAGE_SIZE 4096
#endif

/* the default mutex used if attribute does not declare mutex_type  */
#ifndef ECS_DEFAULT_RESOURCE_MUTEX
#define ECS_DEFAULT_RESOURCE_MUTEX void
#endif

/* the default entity used if component does not declare entity_type  */
 #ifndef ECS_DEFAULT_ENTITY
 #define ECS_DEFAULT_ENTITY ecs::entity
 #endif

/* the default addialize event used if component does not declare init_event  */
#ifndef ECS_DEFAULT_INITIALIZE_EVENT
#define ECS_DEFAULT_INITIALIZE_EVENT ecs::event::initialize<T>
#endif
 
/* the default terminate event used if attribute does not declare term_event  */
#ifndef ECS_DEFAULT_TERMINATE_EVENT  
#define ECS_DEFAULT_TERMINATE_EVENT ecs::event::terminate<T>
#endif
 
/* the default create event used if entity does not declare create_event  */
#ifndef ECS_DEFAULT_CREATE_EVENT
#define ECS_DEFAULT_CREATE_EVENT ecs::event::create<T>
#endif
 
/* the default destroy event used if entity does not declare destroy_event */
#ifndef ECS_DEFAULT_DESTROY_EVENT
#define ECS_DEFAULT_DESTROY_EVENT ecs::event::destroy<T>
#endif

/* macro to ignore commas when passing macro arguments */
#define EXPAND(...) __VA_ARGS__

/**
 * declares trait type getter meta functions that retrieve type TRAIT::NAME, 
 * DEFAULT defines the default type none declared default type is passed 
 */
#define TRAIT_TYPE(NAME, ALIAS, TRAIT)											\
template<TRAIT##_class T>														\
struct get_##NAME { using type = typename TRAIT##_traits<T>::ALIAS; };			\
template<TRAIT##_class T>														\
using get_##NAME##_t = typename get_##NAME<T>::type;

/**
 * declares value getter meta functions that retrieve value TRAIT::NAME 
 * DEFAULT defines the default value none declared 
 */
#define TRAIT_VALUE(TYPE, NAME, ALIAS, TRAIT)									\
template<TRAIT##_class T>														\
struct get_##NAME { static constexpr TYPE value = TRAIT##_traits<T>::ALIAS; };	\
template<TRAIT##_class T>														\
static constexpr TYPE get_##NAME##_v = get_##NAME<T>::value;

/**
 * declares attribute type getter function that retrieve type T::NAME
 * DEFAULT defines the default type none declared
 */
#define TRAIT_ATTRIB_TYPE(NAME, ALIAS, DEFAULT)									\
template<typename T, typename D=DEFAULT> 										\
struct get_attrib_##NAME { using type = D; }; 									\
template<typename T, typename D> requires requires { typename T::ALIAS; }		\
struct get_attrib_##NAME<T, D> { using type = typename T::ALIAS; }; 			\
template<typename T, typename D=DEFAULT>										\
using get_attrib_##NAME##_t = typename get_attrib_##NAME<T, D>::type;

/**
 * declares attribute value getter function that retrieve value T::NAME
 * DEFAULT defines the default value none declared 
 */
#define TRAIT_ATTRIB_VALUE(TYPE, NAME, ALIAS, DEFAULT)							\
template<typename T, TYPE D=DEFAULT> 											\
struct get_attrib_##NAME { static constexpr TYPE value = D; };	  		 	\
template<typename T, TYPE D> requires requires { T::ALIAS; }					\
struct get_attrib_##NAME<T, D> { static constexpr TYPE value = T::ALIAS; };		\
template<typename T, TYPE D=DEFAULT>											\
static constexpr TYPE get_attrib_##NAME##_v = get_attrib_##NAME<T, D>::value;


namespace ecs::traits {
	TRAIT_ATTRIB_TYPE(ecs_category, ecs_category, ECS_DEFAULT_TAG)
}

namespace ecs {
	template<typename T, typename=traits::get_attrib_ecs_category_t<T>> struct attribute_traits;
	template<typename T, typename=traits::get_attrib_ecs_category_t<T>> struct event_traits;
	template<typename T, typename=traits::get_attrib_ecs_category_t<T>> struct entity_traits;
	template<typename T, typename=traits::get_attrib_ecs_category_t<T>> struct component_traits;
}

namespace ecs::traits::attribute {
	/* determines the order in which registery::lock() acquires the mutex of each attribute. */
	TRAIT_ATTRIB_VALUE(int, lock_priority, lock_priority, 0)
	/* determines the order in which registery::lock() acquires the mutex of each attribute. */
	TRAIT_VALUE(int, lock_priority, lock_priority, attribute)
	
	/* determines the order in which registry::registry() initializes each attribute. */
	TRAIT_ATTRIB_VALUE(int, init_priority, init_priority, 0)
	/* determines the order in which registry::registry() initializes each attribute. */
	TRAIT_VALUE(int, init_priority, init_priority, attribute)
	
	/* the type used to lock a attribute. */
	TRAIT_ATTRIB_TYPE(mutex, mutex_type, ECS_DEFAULT_RESOURCE_MUTEX)
	/* the type used to lock a attribute. */
	TRAIT_TYPE(mutex, mutex_type, attribute)

	/* the value of the attribute allows a decoupling of the attribute id type and the value return type. */
	TRAIT_ATTRIB_TYPE(value, value_type, T)
	/* the value of the attribute allows a decoupling of the attribute id type and the value return type. */
	TRAIT_TYPE(value, value_type, attribute)
}

namespace ecs::traits::handle {
	TRAIT_ATTRIB_TYPE(integral, integral_type, std::size_t)
	TRAIT_ATTRIB_VALUE(std::size_t, version_width, version_width, 0)
}

namespace ecs::traits::entity {
	/* the integral of the entity handle */
	TRAIT_TYPE(integral, integral_type, entity)
	
	/* the handle type is the primitive value implementing version width within a integral_type. */
	TRAIT_ATTRIB_TYPE(handle, handle_type, ECS_DEFAULT_HANDLE)
	TRAIT_TYPE(handle, handle_type, entity)

	/* the event fired when an entity is created */
	TRAIT_ATTRIB_TYPE(create_event, create_event, ECS_DEFAULT_CREATE_EVENT)
	TRAIT_TYPE(create_event, create_event, entity)
	
	/* the event fired when an entity is destroyed */
	TRAIT_ATTRIB_TYPE(destroy_event, destroy_event, ECS_DEFAULT_DESTROY_EVENT)
	TRAIT_TYPE(destroy_event, destroy_event, entity)

	/* the factory is an attribute to create new handle indices. */
	TRAIT_ATTRIB_TYPE(factory, factory_type, factory<T>)
	TRAIT_TYPE(factory, factory_type, entity)

	/* the factory is an attribute to create new handle indices. */
	TRAIT_ATTRIB_TYPE(archive, archive_type, archive<T>)
	TRAIT_TYPE(archive, archive_type, entity)


	TRAIT_ATTRIB_VALUE(std::size_t, page_size, page_size, ECS_DEFAULT_PAGE_SIZE)
	TRAIT_VALUE(std::size_t, page_size, page_size, entity)
}

namespace ecs::traits::component {
	/* component value type decouples the component value from the component id type */
	TRAIT_ATTRIB_TYPE(value, value_type, EXPAND(std::conditional_t<std::is_empty_v<T>, void, T>))
	TRAIT_TYPE(value, value_type, component)

	/* component entity type determines the entity table the component belongs to */
	TRAIT_ATTRIB_TYPE(entity, entity_type, ECS_DEFAULT_ENTITY)
	TRAIT_TYPE(entity, entity_type, component)
	
	/* the event fired when a component is initialized */
	TRAIT_ATTRIB_TYPE(initialize_event, initialize_event, ECS_DEFAULT_INITIALIZE_EVENT)
	TRAIT_TYPE(initialize_event, initialize_event, component)
	
	/* the event fired when a component is terminated */
	TRAIT_ATTRIB_TYPE(terminate_event, terminate_event, ECS_DEFAULT_TERMINATE_EVENT)
	TRAIT_TYPE(terminate_event, terminate_event, component)

	/* the manager attribute stores the sequence of entity handles. */
	TRAIT_ATTRIB_TYPE(manager, manager_type, EXPAND(std::conditional_t<std::is_void_v<get_entity_t<T>>, void, manager<T>>))
	TRAIT_TYPE(manager, manager_type, component)

	/* the indexer attribute provides a lookup to the index of the handle */
	TRAIT_ATTRIB_TYPE(indexer, indexer_type, EXPAND(std::conditional_t<std::is_void_v<get_entity_t<T>>, void, indexer<T>>))
	TRAIT_TYPE(indexer, indexer_type, component)

	/* the storage attribute stores the component value_type corresponding to the entity. */
	TRAIT_ATTRIB_TYPE(storage, storage_type, EXPAND(std::conditional_t<std::is_void_v<get_value_t<T>>, void, storage<T>>))
	TRAIT_TYPE(storage, storage_type, component)

	TRAIT_ATTRIB_VALUE(std::size_t, page_size, page_size, ECS_DEFAULT_PAGE_SIZE)
	TRAIT_VALUE(std::size_t, page_size, page_size, component)

	template<typename T> struct get_handle { 
		using type = std::conditional_t<std::is_void_v<get_entity_t<T>>, std::size_t, traits::entity::get_handle_t<get_entity_t<T>>>;
	};
	template<typename T> using get_handle_t = typename get_handle<T>::type;
}

namespace ecs::traits::event {
	/* the callback type decaring the function signature of the event */
	TRAIT_ATTRIB_TYPE(callback, callback_type, void(T&))
	/* the callback type decaring the function signature of the event */
	TRAIT_TYPE(callback, callback_type, event)

	/* the listener type is the function wrapper that fires each individual subscriber to an event */
	TRAIT_ATTRIB_TYPE(listener, listener_type, listener<T>)
	/* the listener type is the function wrapper that fires each individual subscriber to an event */
	TRAIT_TYPE(listener, listener_type, event)
}

// dependencies
namespace ecs::traits::dependencies {
	TRAIT_ATTRIB_TYPE(dependencies, dependency_set, std::tuple<>)

	template<typename T> struct get_dependencies;
	template<typename T> using get_dependencies_t = typename get_dependencies<T>::type;
	
	template<attribute_class T> struct get_dependencies<T> { 
		using type = get_attrib_dependencies_t<T>; 
	};

	template<entity_class T> struct get_dependencies<T> { 
		using type = util::push_back_t<get_attrib_dependencies_t<T>, 
			traits::entity::get_factory_t<T>, 
			traits::entity::get_create_event_t<T>, 
			traits::entity::get_destroy_event_t<T>
		>; 
	};
	
	template<component_class T> struct get_dependencies<T> { 
		using type = util::push_back_t<get_attrib_dependencies_t<T>, 
			traits::component::get_initialize_event_t<T>, 
			traits::component::get_terminate_event_t<T>, 
			traits::component::get_entity_t<T>, 
			traits::component::get_manager_t<T>, 
			traits::component::get_indexer_t<T>, 
			traits::component::get_storage_t<T>
		>;
	};
	
	template<event_class T> struct get_dependencies<T> { 
		using type = util::push_back_t<get_attrib_dependencies_t<T>, 
			traits::event::get_listener_t<T>
		>; 
	};

	namespace details {	
		template<typename Tup, typename Out=std::tuple<>>
		struct get_dependency_set {
		private:
			using set = util::filter_t<Tup, util::pred::disj_<util::pred::element_of_<Out>::template type, std::is_void>::template inv>;
			using dep = util::unique_t<util::filter_t<util::concat_t<util::eval_each_t<Tup, dependencies::get_dependencies>>, util::cmp::to_<void>::template inv>>;
			using out = util::concat_t<std::tuple<Out, set>>;
			
			static constexpr auto calc_dependencies() {
				if constexpr (!ECS_RECURSIVE_DEPENDENCY) {
					return std::type_identity<dep>{};
				} else if constexpr (std::is_same_v<std::tuple<>, set>) {
					return std::type_identity<Out>{};
				} else {
					return std::type_identity<typename ecs::traits::dependencies::details::get_dependency_set<dep, out>::type>{};
				}
			}	
		public:
			using type = decltype(calc_dependencies())::type;
		};
	}

	template<typename ... Ts> using get_dependency_set = details::get_dependency_set<std::tuple<Ts...>>;
	template<typename ... Ts> using get_dependency_set_t = typename get_dependency_set<Ts...>::type;

	template<typename ... Ts> struct get_attribute_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_attribute>; };
	template<typename ... Ts> using get_attribute_set_t = typename get_attribute_set<Ts...>::type;

	template<typename ... Ts> struct get_entity_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_entity>; };
	template<typename ... Ts> using get_entity_set_t = typename get_entity_set<Ts...>::type;

	template<typename ... Ts> struct get_component_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_component>; };
	template<typename ... Ts> using get_component_set_t = typename get_component_set<Ts...>::type;

	template<typename ... Ts> struct get_event_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_event>; };
	template<typename ... Ts> using get_event_set_t = typename get_event_set<Ts...>::type;
} // ecs::traits::dependencies

namespace ecs::traits {
	namespace details {
		template<typename T, typename Tags> using tag_match = util::pred::element_of<get_attrib_ecs_category_t<T>, Tags, util::cmp::disj_<std::is_same, std::is_base_of>::template type>;
	}
	template<typename T> struct is_attribute : details::tag_match<T, std::tuple<tag::attribute>> { };
	template<typename T> struct is_entity : details::tag_match<T, std::tuple<tag::entity>> { }; 
	template<typename T> struct is_component : details::tag_match<T, std::tuple<tag::component, tag::flag, tag::unit, tag::archetype<>, tag::uniontype<>>> { }; 
	template<typename T> struct is_event : details::tag_match<T, std::tuple<tag::event>> { };
}

#undef EXPAND
#undef TRAIT_TYPE
#undef TRAIT_VALUE
#undef TRAIT_FUNC
#undef TRAIT_ATTRIB_TYPE
#undef TRAIT_ATTRIB_VALUE


namespace ecs {
	template<typename T>
	struct attribute_traits<T, tag::attribute> {
		using mutex_type = traits::attribute::get_attrib_mutex_t<T>;
		using value_type = traits::attribute::get_attrib_value_t<T>;
		
		static constexpr int lock_priority = traits::attribute::get_attrib_lock_priority_v<T>;
		static constexpr int init_priority = traits::attribute::get_attrib_init_priority_v<T>;
	};

	template<typename T>
	struct entity_traits<T, tag::entity> {
		using handle_type = traits::entity::get_attrib_handle_t<T>;
		using integral_type = traits::handle::get_attrib_integral_t<handle_type>;
		
		using create_event = traits::entity::get_attrib_create_event_t<T>;
		using destroy_event = traits::entity::get_attrib_destroy_event_t<T>;
		
		using factory_type = traits::entity::get_attrib_factory_t<T, factory<T>>; 
		using archive_type = traits::entity::get_attrib_archive_t<T, archive<T>>;
		static constexpr std::size_t page_size = traits::entity::get_attrib_page_size_v<T>;
	};

	struct pointer_handle {
		using ecs_category = ecs::tag::entity;
		using handle_type = void*;
	private:
		void* data;
	};

	template<typename T>
	struct component_traits<T, tag::component> {
		using value_type = traits::component::get_attrib_value_t<T>;
		using entity_type = traits::component::get_attrib_entity_t<T>;

		using initialize_event = traits::component::get_attrib_initialize_event_t<T>;
		using terminate_event = traits::component::get_attrib_terminate_event_t<T>;

		using indexer_type = traits::component::get_attrib_indexer_t<T, indexer<T>>;
		using manager_type = traits::component::get_attrib_manager_t<T, std::conditional_t<std::is_void_v<entity_type>, void, manager<T>>>;
		using storage_type = traits::component::get_attrib_storage_t<T, std::conditional_t<std::is_void_v<value_type>, void, storage<T>>>;

		static constexpr std::size_t page_size = traits::component::get_attrib_page_size_v<T>;
	};


	template<typename T>
	struct event_traits<T, tag::event> {
		using callback_type = traits::event::get_attrib_callback_t<T>;
		using listener_type = traits::event::get_attrib_listener_t<T, listener<T>>;
		using dependency_set = util::push_back_t<traits::dependencies::get_attrib_dependencies_t<T>, listener_type>;
	};
}