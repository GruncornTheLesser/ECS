#pragma once
#include "core/fwd.h"
#include "core/macros.h"
#include <util.h>

namespace ecs::traits {
	ATTRIB_TYPE(ecs_tag, ecs_tag, ECS_DEFAULT_TAG)
}

namespace ecs {
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct resource_traits;
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct event_traits;
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct entity_traits;
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct component_traits;
}

namespace ecs::traits::resource {
	/* determines the order in which registery::lock() acquires the mutex of each resource. */
	ATTRIB_VALUE(int, lock_priority, lock_priority, 0)
	/* determines the order in which registery::lock() acquires the mutex of each resource. */
	TRAIT_VALUE(int, lock_priority, lock_priority, resource)
	
	/* determines the order in which registry::registry() initializes each resource. */
	ATTRIB_VALUE(int, init_priority, init_priority, 0)
	/* determines the order in which registry::registry() initializes each resource. */
	TRAIT_VALUE(int, init_priority, init_priority, resource)
	
	/* the type used to lock a resource. */
	ATTRIB_TYPE(mutex, mutex_type, ECS_DEFAULT_RESOURCE_MUTEX)
	/* the type used to lock a resource. */
	TRAIT_TYPE(mutex, mutex_type, resource)

	/* the value of the resource allows a decoupling of the resource id type and the value return type. */
	ATTRIB_TYPE(value, value_type, T)
	/* the value of the resource allows a decoupling of the resource id type and the value return type. */
	TRAIT_TYPE(value, value_type, resource)
}

namespace ecs::traits::event {
	/* the callback type decaring the function signature of the event */
	ATTRIB_TYPE(callback, callback_type, void(T&))
	/* the callback type decaring the function signature of the event */
	TRAIT_TYPE(callback, callback_type, event)

	/* the listener type is the function wrapper that fires each individual subscriber to an event */
	ATTRIB_TYPE(listener, listener_type, listener<T>)
	/* the listener type is the function wrapper that fires each individual subscriber to an event */
	TRAIT_TYPE(listener, listener_type, event)

	/* fire once allows a bool fire once to be passed when attaching a listener to an event dispatcher.  */
	ATTRIB_VALUE(bool, enable_fire_once, enable_fire_once, false)
	/* fire once allows a bool fire once to be passed when attaching a listener to an event dispatcher.  */
	TRAIT_VALUE(bool, enable_fire_once, enable_fire_once, event)

	/* strict order enforces the order of attachment equals the order of listeners being executed */
	ATTRIB_VALUE(bool, strict_order, strict_order, false)
	/* strict order enforces the order of attachment equals the order of listeners being executed */
	TRAIT_VALUE(bool, strict_order, strict_order, event)
}

namespace ecs::traits::entity {
	/* the integral of the entity handle */
	ATTRIB_TYPE(integral, integral_type, ECS_DEFAULT_HANDLE_INTEGRAL)
	/* the number of bits dedicated to the version. */
	ATTRIB_VALUE(std::size_t, version_width,  version_width, ECS_DEFAULT_VERSION_WIDTH)
	/* the handle type is the primitive value implementing version width within a integral_type. */
	TRAIT_TYPE(handle, handle_type, entity)

	/* the page size defines the capacity of each page for packed and sparse pages */
	ATTRIB_VALUE(std::size_t, page_size, page_size, ECS_DEFAULT_PAGE_SIZE)
	/* the page size defines the capacity of each page for packed and sparse pages */
	TRAIT_VALUE(std::size_t, page_size, page_size, entity)

	/* the event fired when an entity is created */
	ATTRIB_TYPE(create_event, create_event, ECS_DEFAULT_CREATE_EVENT)
	/* the event fired when an entity is created */
	TRAIT_TYPE(create_event, create_event, entity)
	
	/* the event fired when an entity is destroyed */
	ATTRIB_TYPE(destroy_event, destroy_event, ECS_DEFAULT_DESTROY_EVENT)
	/* the event fired when an entity is destroyed */
	TRAIT_TYPE(destroy_event, destroy_event, entity)

	/* the factory type is declared in the type traits. the factory is a resource to store manage unique handle creation and destruction */
	TRAIT_TYPE(factory, factory_type, entity)
}

namespace ecs::traits::component {
	/* component value type decouples the component value from the component id type */
	ATTRIB_TYPE(value, value_type, T)
	/* component value type decouples the component value from the component id type */
	TRAIT_TYPE(value, value_type, component)

	/* component entity type determines the entity table the component belongs to. */
	ATTRIB_TYPE(entity, entity_type, ECS_DEFAULT_ENTITY)
	/* component entity type determines the entity table the component belongs to. */
	TRAIT_TYPE(entity, entity_type, component)
	
	/* the event fired when a component is initialized */
	ATTRIB_TYPE(initialize_event, initialize_event, ECS_DEFAULT_INITIALIZE_EVENT)
	/* the event fired when a component is initialized */
	TRAIT_TYPE(initialize_event, initialize_event, component)
	
	/* the event fired when a component is terminated */
	ATTRIB_TYPE(terminate_event, terminate_event, ECS_DEFAULT_TERMINATE_EVENT)
	/* the event fired when a component is terminated */
	TRAIT_TYPE(terminate_event, terminate_event, component)

	/* the manager resource stores the sequence of entity handles. it must implements... */
	TRAIT_TYPE(manager, manager_type, component)
	/* the indexer resource stores the index lookup into the mnager sequence. it must implement ... */
	TRAIT_TYPE(indexer, indexer_type, component)
	/* the storage resource stores the component value_type corresponding to the entity. it must implement ... */
	TRAIT_TYPE(storage, storage_type, component)
}

namespace ecs::traits::registry {
	
	ATTRIB_TYPE(resource_set, resource_set, std::tuple<>)
	ATTRIB_TYPE(entity_set, entity_set, std::tuple<>)
	ATTRIB_TYPE(component_set, component_set, std::tuple<>)
	ATTRIB_TYPE(event_set, event_set, std::tuple<>)
}
// dependencies
namespace ecs::traits::dependencies {
	ATTRIB_TYPE(dependency_set, dependency_set, std::tuple<>)

	#if ECS_RECURSIVE_DEPENDENCY
	namespace details {	
		template<typename Tup, typename Out=std::tuple<>>
		struct get_dependency_set;
		
		template<typename ... Ts, typename Out>
		struct get_dependency_set<std::tuple<Ts...>, Out> {
		private:
			using set = util::filter_t<std::tuple<Ts...>, util::pred::element_of_<Out>::template inv>;
			using dep = util::unique_t<util::filter_t<util::concat_t<std::tuple<typename ecs::traits::get_traits_t<Ts>::dependency_set...>>, util::cmp::to_<void>::template inv>>;
			using out = util::concat_t<std::tuple<Out, set>>;
		public:
			using type = std::conditional_t<std::is_same_v<std::tuple<>, set>, Out, typename get_dependency_set<dep, out>::type>;
		};

		template<typename ... Ts, typename Out>
		struct get_dependency_set<std::tuple<void, Ts...>, Out> : get_dependency_set<std::tuple<Ts...>, Out> { };
	
		template<typename Out>
		struct get_dependency_set<std::tuple<>, Out> { 
			using type = Out;
		};
	}
	template<typename ... Ts> using get_dependency_set = details::get_dependency_set<std::tuple<Ts...>>;
	#else 
	template<typename ... Ts> using get_dependency_set = util::concat<std::tuple<typename ecs::traits::get_traits_t<Ts>::dependency_set...>>;
	#endif
	template<typename ... Ts> using get_dependency_set_t = typename get_dependency_set<Ts...>::type;


	template<typename ... Ts> struct get_resource_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_resource>; };
	template<typename ... Ts> using get_resource_set_t = typename get_resource_set<Ts...>::type;

	template<typename ... Ts> struct get_entity_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_entity>; };
	template<typename ... Ts> using get_entity_set_t = typename get_entity_set<Ts...>::type;

	template<typename ... Ts> struct get_component_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_component>; };
	template<typename ... Ts> using get_component_set_t = typename get_component_set<Ts...>::type;

	template<typename ... Ts> struct get_event_set { using type = util::filter_t<get_dependency_set_t<Ts...>, is_event>; };
	template<typename ... Ts> using get_event_set_t = typename get_event_set<Ts...>::type;

	
}

namespace ecs::traits {
	//util::pred::anyof_v<comp_res_set, util::cmp::to_<int, util::cmp::is_const_accessible>::template type>
	template<typename reg_T, traits::resource_class T> struct is_accessible<reg_T, T>
	 : util::pred::element_of<T, traits::registry::get_attrib_resource_set_t<reg_T>, util::cmp::is_const_accessible> { };

	template<typename reg_T, traits::entity_class T> struct is_accessible<reg_T, T>
	 : util::pred::element_of<T, traits::registry::get_attrib_entity_set_t<reg_T>, util::cmp::is_const_accessible> { };

	 template<typename reg_T, traits::component_class T> struct is_accessible<reg_T, T>
	  : util::pred::element_of<T, traits::registry::get_attrib_component_set_t<reg_T>, util::cmp::is_const_accessible> { };

	template<typename reg_T, traits::event_class T> struct is_accessible<reg_T, T>
	 : util::pred::element_of<T, traits::registry::get_attrib_event_set_t<reg_T>, util::cmp::is_const_accessible> { };
}

namespace ecs::traits {
	template<typename T> struct is_resource : util::cmp::disj<ecs::tag::resource, ecs::traits::get_attrib_ecs_tag_t<T>, std::is_same, std::is_base_of> { };
	template<typename T> struct is_entity : util::cmp::disj<ecs::tag::entity, ecs::traits::get_attrib_ecs_tag_t<T>, std::is_same, std::is_base_of> { };
	template<typename T> struct is_component : util::cmp::disj<ecs::tag::component, ecs::traits::get_attrib_ecs_tag_t<T>, std::is_same, std::is_base_of> { };
	template<typename T> struct is_event : util::cmp::disj<ecs::tag::event, ecs::traits::get_attrib_ecs_tag_t<T>, std::is_same, std::is_base_of> { };
	
	template<typename T> struct get_traits<T, std::enable_if_t<is_resource_v<T>>> { using type = resource_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_entity_v<T>>> { using type = entity_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_component_v<T>>> { using type = component_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_event_v<T>>> { using type = event_traits<T>; };
}