#pragma once
#include "core/traits.h"
#include <variant>

// component traits
namespace ecs {
	template<typename T>
	struct component_traits<T, tag::component> {
		using value_type = traits::component::get_attrib_value_t<T>;
		using entity_type = traits::component::get_attrib_entity_t<T>;
		
		using manager_type = traits::component::get_attrib_manager_t<T>;
		using indexer_type = traits::component::get_attrib_indexer_t<T>;
		using storage_type = traits::component::get_attrib_storage_t<T>;
		
		using init_event = traits::component::get_attrib_init_event_t<T>;
		using term_event = traits::component::get_attrib_term_event_t<T>;
		
		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>, 
			init_event, term_event, entity_type, manager_type, indexer_type, storage_type>;
	};

	template<typename T, typename ... Ts>
	struct component_traits<T, tag::archetype<Ts...>> {
		using value_type = traits::component::get_attrib_value_t<T>;
		using entity_type = traits::component::get_attrib_entity_t<T>;
		
		using manager_type = traits::component::get_attrib_manager_t<T>;
		using indexer_type = traits::component::get_attrib_indexer_t<T>;
		using storage_type = traits::component::get_attrib_storage_t<T>;
		
		using init_event = traits::component::get_attrib_init_event_t<T>;
		using term_event = traits::component::get_attrib_term_event_t<T>;

		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>, 
			init_event, term_event, entity_type, manager_type, indexer_type, storage_type>;
	};

	template<typename T, typename ... Ts>
	struct component_traits<T, tag::uniontype<Ts...>> {
	private:
		using default_value_type = util::sort_t<std::variant<Ts...>, util::cmp::lt_<>::template type>;
	public:
		using value_type = traits::component::get_attrib_value_t<T, default_value_type>;
		using entity_type = traits::component::get_attrib_entity_t<T>;
		
		using manager_type = traits::component::get_attrib_manager_t<T>;
		using indexer_type = traits::component::get_attrib_indexer_t<T>;
		using storage_type = traits::component::get_attrib_storage_t<T>;

		using init_event = traits::component::get_attrib_init_event_t<default_value_type>;
		using term_event = traits::component::get_attrib_term_event_t<default_value_type>;

		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>, 
			init_event, term_event, entity_type, manager_type, indexer_type, storage_type>;
	};
}

namespace ecs {
    template<ecs::traits::event_class T> 
	struct listener {
		using ecs_tag = ecs::tag::component;
		using entity_type = ecs::event_entity<T>;
		using value_type = traits::event::get_callback_t<T>;
		using init_event = void;
		using term_event = void;
	};

}