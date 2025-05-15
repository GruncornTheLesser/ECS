#pragma once
#include "core/traits.h"
#include <variant>

// component traits
namespace ecs {
	template<typename T>
	struct component_traits<T, tag::component_basictype> {
		using value_type = traits::component::get_attrib_value_t<T>;
		using entity_type =  traits::component::get_attrib_entity_t<T>;
		using manager_type = traits::component::get_attrib_manager_t<T>;
		using indexer_type = traits::component::get_attrib_indexer_t<T>;
		using storage_type = traits::component::get_attrib_storage_t<T, std::conditional_t<std::is_empty_v<value_type>, void, storage<T>>>;
		using factory_type = typename entity_traits<entity_type>::factory_type;
		using resource_dependencies = std::tuple<manager_type, indexer_type, storage_type, factory_type>;
		using component_dependencies = std::tuple<T>;
		using entity_dependencies = std::tuple<entity_type>;
	};

	template<typename T, typename ... Ts>
	struct component_traits<T, tag::component_archetype<Ts...>> {
	private:
		using primary_type = meta::min_by_t<std::tuple<Ts...>, meta::get_type_name>;
	public:
		using value_type = traits::component::get_attrib_value_t<T>;
		using entity_type = traits::component::get_attrib_entity_t<primary_type>;
		using manager_type = traits::component::get_attrib_manager_t<T, manager<primary_type>>;
		using indexer_type = traits::component::get_attrib_indexer_t<T, indexer<primary_type>>;
		using storage_type = traits::component::get_attrib_storage_t<T, std::conditional_t<std::is_empty_v<value_type>, void, storage<T>>>;
		using factory_type = typename entity_traits<entity_type>::factory_type;
		//using init_event_type = traits::component::get_init_event_t<T>;
		//using term_event_type = traits::component::get_term_event_t<T>;
		

		using resource_dependencies = std::tuple<manager_type, indexer_type, typename component_traits<Ts>::storage_type..., factory_type>;
		using component_dependencies = std::tuple<Ts...>;
		using entity_dependencies = std::tuple<entity_type>;
	};

	template<typename T, typename ... Ts>
	struct component_traits<T, tag::component_uniontype<Ts...>> {
	private:
		using primary_type = meta::sort_by_t<std::variant<Ts...>, meta::get_type_name>;
	public:
		using value_type = traits::component::get_attrib_value_t<T, std::variant<Ts...>>;
		using entity_type = traits::component::get_attrib_entity_t<T>;
		using manager_type = traits::component::get_attrib_manager_t<T, manager<primary_type>>;
		using indexer_type = traits::component::get_attrib_indexer_t<T, indexer<primary_type>>;
		using storage_type = traits::component::get_attrib_storage_t<T, std::conditional_t<std::is_empty_v<value_type>, void, storage<primary_type>>>;
		using factory_type = typename entity_traits<entity_type>::factory_type;
		using init_event_type = traits::component::get_init_event_t<T>;
		using term_event_type = traits::component::get_term_event_t<T>;
		

		using resource_dependencies = std::tuple<manager_type, indexer_type, storage_type, factory_type>;
		using component_dependencies = std::tuple<Ts...>;
		using entity_dependencies = std::tuple<entity_type>;
	};
}

namespace ecs {
    template<ecs::traits::event_class T, typename callback_T> 
	struct listener : callback_T { 
		using ecs_tag_type = ecs::tag::component_basictype;
		using entity_type = ecs::event_entity<T>;
	};

}