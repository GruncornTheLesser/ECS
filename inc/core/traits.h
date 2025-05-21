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
	ATTRIB_VALUE(int, lock_priority, lock_priority, 0)
	TRAIT_VALUE(int, lock_priority, lock_priority, resource)
	
	ATTRIB_VALUE(int, init_priority, init_priority, 0)
	TRAIT_VALUE(int, init_priority, init_priority, resource)
	
	ATTRIB_TYPE(mutex, mutex_type, ECS_DEFAULT_RESOURCE_MUTEX)
	TRAIT_TYPE(mutex, mutex_type, resource)

	ATTRIB_TYPE(value, value_type, T)
	TRAIT_TYPE(value, value_type, resource)

	ATTRIB_TYPE(acquire_event, acquire_event, ECS_DEFAULT_ACQUIRE_EVENT)
	TRAIT_TYPE(acquire_event, acquire_event, entity)
	
	ATTRIB_TYPE(release_event, release_event, ECS_DEFAULT_RELEASE_EVENT)
	TRAIT_TYPE(release_event, release_event, entity)
}

namespace ecs::traits::event {
	ATTRIB_TYPE(callback, callback_type, void(T&))
	TRAIT_TYPE(callback, callback_type, event)

	ATTRIB_TYPE(listener, listener_type, listener<T>)
	TRAIT_TYPE(listener, listener_type, event)

	ATTRIB_VALUE(bool, strict_order, strict_order, false)
	TRAIT_VALUE(bool, strict_order, strict_order, event)

	ATTRIB_VALUE(bool, enable_async, enable_async, false)
	TRAIT_VALUE(bool, enable_async, enable_async, event)
}

namespace ecs::traits::entity {
	ATTRIB_TYPE(integral, integral_type, ECS_DEFAULT_HANDLE_INTEGRAL)
	ATTRIB_VALUE(std::size_t, version_width,  version_width, ECS_DEFAULT_VERSION_WIDTH)
	TRAIT_TYPE(handle, handle_type, entity)

	ATTRIB_TYPE(create_event, create_event, ECS_DEFAULT_CREATE_EVENT)
	TRAIT_TYPE(create_event, create_event, entity)
	
	ATTRIB_TYPE(destroy_event, destroy_event, ECS_DEFAULT_DESTROY_EVENT)
	TRAIT_TYPE(destroy_event, destroy_event, entity)

	TRAIT_TYPE(factory, factory_type, entity)
}

namespace ecs::traits::component {
	ATTRIB_TYPE(value, value_type, T)
	TRAIT_TYPE(value, value_type, component)

	ATTRIB_TYPE(entity, entity_type, ECS_DEFAULT_ENTITY)
	TRAIT_TYPE(entity, entity_type, component)
	
	ATTRIB_TYPE(init_event, init_event, ECS_DEFAULT_INITIALIZE_EVENT)
	TRAIT_TYPE(init_event, init_event, component)
	
	ATTRIB_TYPE(term_event, term_event, ECS_DEFAULT_TERMINATE_EVENT)
	TRAIT_TYPE(term_event, term_event, component)

	ATTRIB_TYPE(manager, manager_type, manager<T>)
	TRAIT_TYPE(manager, manager_type, component)

	ATTRIB_TYPE(indexer, indexer_type, indexer<T>)
	TRAIT_TYPE(indexer, indexer_type, component)

	ATTRIB_TYPE(storage, storage_type, storage<T>)
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
	
		template<typename Out>
		struct get_dependency_set<std::tuple<>, Out> { 
			using type = util::unique_t<Out>;
		};
	}
	template<typename ... Ts> using get_dependency_set = details::get_dependency_set<std::tuple<Ts...>>;
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

// concepts definitions
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