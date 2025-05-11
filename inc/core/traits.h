#pragma once
#include "core/fwd.h"
#include "core/meta.h"


namespace ecs {
	// traits
	template<typename T, typename=traits::get_ecs_tag_t<T>> struct resource_traits;
	template<typename T, typename=traits::get_ecs_tag_t<T>> struct event_traits;
	template<typename T, typename=traits::get_ecs_tag_t<T>> struct entity_traits;
	template<typename T, typename=traits::get_ecs_tag_t<T>> struct component_traits;

	template<typename T> struct resource_traits<T, ecs::tag::resource> : resource_traits<T, ECS_DEFAULT_RESOURCE_TAG> { };
	template<typename T> struct entity_traits<T, ecs::tag::entity> : entity_traits<T, ECS_DEFAULT_ENTITY_TAG> { };
	template<typename T> struct component_traits<T, ecs::tag::component> : component_traits<T, ECS_DEFAULT_COMPONENT_TAG> { };
	template<typename T> struct event_traits<T, ecs::tag::event> : event_traits<T, ECS_DEFAULT_EVENT_TAG> { };
}

namespace ecs::traits::resource {
	DYNAMIC_TRAIT_VALUE(int, lock_priority, 0)
	DYNAMIC_TRAIT_VALUE(int, init_priority, 0)
	DYNAMIC_TRAIT_TYPE(mutex, null_mutex)
	DYNAMIC_TRAIT_TYPE(value, T)
	STATIC_TRAIT_TYPE(cache, resource)
}

namespace ecs::traits::event {
	DYNAMIC_TRAIT_TYPE(callback, void(T&))
	DYNAMIC_TRAIT_VALUE(bool, strict_order, false)
	DYNAMIC_TRAIT_VALUE(bool, enable_async, false)
}

namespace ecs::traits::entity {
	DYNAMIC_TRAIT_TYPE(handle_integral, ECS_DEFAULT_HANDLE_INTEGRAL)
	DYNAMIC_TRAIT_VALUE(std::size_t, handle_version_width, ECS_DEFAULT_HANDLE_VERSION_WIDTH)
	STATIC_TRAIT_TYPE(handle, entity)
	DYNAMIC_TRAIT_TYPE(factory, factory<T>)
}
	
namespace ecs::traits::component {
	DYNAMIC_TRAIT_TYPE(entity, ecs::entity)
	DYNAMIC_TRAIT_TYPE(manager, manager<T>)
	DYNAMIC_TRAIT_TYPE(indexer, indexer<T>)
	DYNAMIC_TRAIT_TYPE(storage, EXPAND(std::conditional_t<std::is_empty_v<T>, void, storage<T>>))
	DYNAMIC_TRAIT_TYPE(init_event, ECS_DEFAULT_INITIALIZE_EVENT)
	DYNAMIC_TRAIT_TYPE(term_event, ECS_DEFAULT_TERMINATE_EVENT)
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
	template<typename T, typename U> struct is_const_accessible : std::disjunction<std::is_void<T>, std::is_same<T, U>, std::is_same<T, const U>> { };

	template<typename T, typename reg_T> struct is_accessible : meta::conjunction<ecs::traits::get_resource_dependencies_t<T>, is_accessible, reg_T> { };
	template<resource_class T, typename reg_T> struct is_accessible<T, reg_T> : meta::disjunction<typename reg_T::resource_set, is_const_accessible, T> { };
}

// concepts definitions
namespace ecs::traits {
	template<typename T> struct is_resource : std::disjunction<
		std::is_same<tag::resource, traits::get_ecs_tag_t<T>>, 
		std::is_base_of<tag::resource, traits::get_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct is_entity : std::disjunction<
		std::is_same<tag::entity, traits::get_ecs_tag_t<T>>, 
		std::is_base_of<tag::entity, traits::get_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct is_component : std::disjunction<
		std::is_same<tag::component, traits::get_ecs_tag_t<T>>, 
		std::is_base_of<tag::component, traits::get_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct is_event : std::disjunction<
		std::is_same<tag::event, traits::get_ecs_tag_t<T>>, 
		std::is_base_of<tag::event, traits::get_ecs_tag_t<T>>> 
	{ };
	
	template<typename T> struct get_traits<T, std::enable_if_t<is_resource_v<T>>> { using type = resource_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_event_v<T>>> { using type = event_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_entity_v<T>>> { using type = entity_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_component_v<T>>> { using type = component_traits<T>; };
}