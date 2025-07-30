#pragma once
#include "core/traits.h"
// component traits

namespace ecs {
	template<typename T>
	struct component_traits<T, tag::component> {
		using value_type = traits::component::get_attrib_value_t<T>;
		using entity_type = traits::component::get_attrib_entity_t<T>;
		
		using initialize_event = traits::component::get_attrib_initialize_event_t<T>;
		using terminate_event = traits::component::get_attrib_terminate_event_t<T>;

		using manager_type = std::conditional_t<std::is_void_v<entity_type>, void, manager<T>>;
		using indexer_type = indexer<T>;
		using storage_type = std::conditional_t<std::is_void_v<value_type>, void, storage<T>>;
		
		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>, 
			initialize_event, terminate_event, entity_type, manager_type, indexer_type, storage_type>;
	};

	// template<typename T, typename ... Ts>
	// struct component_traits<T, tag::archetype<Ts...>>;
	// template<typename T, typename ... Ts>
	// struct component_traits<T, tag::uniontype<Ts...>>;
}

namespace ecs {
    template<ecs::traits::event_class T> 
	struct listener {
		using ecs_tag = ecs::tag::component;
		
		using value_type = traits::event::get_callback_t<T>;
		using entity_type = ecs::event_entity<T>;
		using initialize_event = void;
		using terminate_event = void;
	};
}