#pragma once
#include "core/fwd.h"
#include "core/macros.h"
#include "core/meta.h"


namespace ecs::traits {
	ATTRIB_TYPE(ecs_tag, ECS_DEFAULT_TAG)
}

namespace ecs {
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct resource_traits;
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct event_traits;
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct entity_traits;
	template<typename T, typename=traits::get_attrib_ecs_tag_t<T>> struct component_traits;

	template<typename T> struct resource_traits<T, ecs::tag::resource> : resource_traits<T, ECS_DEFAULT_RESOURCE_TAG> { };
	template<typename T> struct entity_traits<T, ecs::tag::entity> : entity_traits<T, ECS_DEFAULT_ENTITY_TAG> { };
	template<typename T> struct component_traits<T, ecs::tag::component> : component_traits<T, ECS_DEFAULT_COMPONENT_TAG> { };
	template<typename T> struct event_traits<T, ecs::tag::event> : event_traits<T, ECS_DEFAULT_EVENT_TAG> { };
}

namespace ecs::traits::resource {
	ATTRIB_VALUE(int, lock_priority, 0)
	TRAIT_VALUE(resource, int, lock_priority)
	
	ATTRIB_VALUE(int, init_priority, 0)
	TRAIT_VALUE(resource, int, init_priority)
	
	ATTRIB_TYPE(mutex, ECS_DEFAULT_RESOURCE_MUTEX)
	TRAIT_TYPE(resource, mutex)

	ATTRIB_TYPE(value, T)
	TRAIT_TYPE(resource, value)
	
	template<typename T> struct get_cache { using type = cache<T>; };
	template<typename T> using get_cache_t = typename get_cache<T>::type;
}

namespace ecs::traits::event {
	ATTRIB_TYPE(callback, void(T&))
	TRAIT_TYPE(event, callback)

	ATTRIB_VALUE(bool, strict_order, false)
	TRAIT_VALUE(event, bool, strict_order)

	ATTRIB_VALUE(bool, enable_async, false)
	TRAIT_VALUE(event, bool, enable_async)
}

namespace ecs::traits::entity {
	ATTRIB_TYPE(integral, ECS_DEFAULT_HANDLE_INTEGRAL)
	TRAIT_TYPE(entity, integral)

	ATTRIB_VALUE(std::size_t, version_width, ECS_DEFAULT_VERSION_WIDTH)
	TRAIT_VALUE(entity, std::size_t, version_width)

	ATTRIB_TYPE(factory, factory<T>)
	TRAIT_TYPE(entity, factory)
	
	TRAIT_TYPE(entity, handle)
}
	
namespace ecs::traits::component {
	ATTRIB_TYPE(value, T)
	TRAIT_TYPE(component, value)

	ATTRIB_TYPE(entity, ecs::entity)
	TRAIT_TYPE(component, entity)

	ATTRIB_TYPE(manager, manager<T>)
	TRAIT_TYPE(component, manager)
	
	ATTRIB_TYPE(indexer, indexer<T>)
	TRAIT_TYPE(component, indexer)
	
	ATTRIB_TYPE(storage, storage<T>)
	TRAIT_TYPE(component, storage)
	
	ATTRIB_TYPE(init_event, ECS_DEFAULT_INITIALIZE_EVENT)
	TRAIT_TYPE(component, init_event)
	
	ATTRIB_TYPE(term_event, ECS_DEFAULT_TERMINATE_EVENT)
	TRAIT_TYPE(component, term_event)
}

// dependencies
namespace ecs::traits {
	template<typename ... Ts> struct get_resource_dependencies { 
		using type = meta::remove_if_t<meta::unique_t<meta::concat_t<typename get_traits_t<Ts>::resource_dependencies...>>, std::is_void>;
	};
	template<typename ... Ts> struct get_entity_dependencies { 
		using type = meta::unique_t<meta::remove_if_t<meta::concat_t<typename get_traits_t<Ts>::entity_dependencies...>, std::is_void>>;
	};
	template<typename ... Ts> struct get_component_dependencies { 
		using type = meta::unique_t<meta::remove_if_t<meta::concat_t<typename get_traits_t<Ts>::component_dependencies...>, std::is_void>>;
	};
}

namespace ecs::traits {
	template<typename T> struct is_data_component : std::negation<std::is_void<meta::eval_try_t<T, traits::component::get_storage>>> { };

	template<typename T, typename manager_T> struct is_manager_match : std::is_same<meta::eval_try_t<T, traits::component::get_manager>, manager_T> { };
	template<typename T, typename indexer_T> struct is_indexer_match : std::is_same<meta::eval_try_t<T, traits::component::get_indexer>, indexer_T> { };
	template<typename T, typename storage_T> struct is_storage_match : std::is_same<meta::eval_try_t<T, traits::component::get_storage>, storage_T> { };
	template<typename T, typename entity_T>  struct is_entity_match  : std::is_same<meta::eval_try_t<T, traits::component::get_entity>,  entity_T>  { };
	
	template<typename T, typename U> struct is_const_accessible : std::disjunction<std::is_void<T>, std::is_same<T, U>, std::is_same<T, const U>> { };

	template<typename T, typename reg_T> struct is_accessible : meta::conjunction<ecs::traits::get_resource_dependencies_t<T>, is_accessible, reg_T> { };
	template<resource_class T, typename reg_T> struct is_accessible<T, reg_T> : meta::disjunction<typename reg_T::resource_set, is_const_accessible, T> { };
}

// concepts definitions
namespace ecs::traits {
	template<typename T> struct is_resource : std::disjunction<
		std::is_same<tag::resource, traits::get_attrib_ecs_tag_t<T>>, 
		std::is_base_of<tag::resource, traits::get_attrib_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct is_entity : std::disjunction<
		std::is_same<tag::entity, traits::get_attrib_ecs_tag_t<T>>, 
		std::is_base_of<tag::entity, traits::get_attrib_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct is_component : std::disjunction<
		std::is_same<tag::component, traits::get_attrib_ecs_tag_t<T>>, 
		std::is_base_of<tag::component, traits::get_attrib_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct is_event : std::disjunction<
		std::is_same<tag::event, traits::get_attrib_ecs_tag_t<T>>, 
		std::is_base_of<tag::event, traits::get_attrib_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct get_traits<T, std::enable_if_t<is_resource_v<T>>> { using type = resource_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_event_v<T>>> { using type = event_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_entity_v<T>>> { using type = entity_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_component_v<T>>> { using type = component_traits<T>; };
}