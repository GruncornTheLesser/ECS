#pragma once 
#include "core/traits.h"
#include <vector>
#include <unordered_map>

// resource traits
namespace ecs {
	template<typename T>
	struct resource_traits<T, ecs::tag::resource> {
		
		using mutex_type = traits::resource::get_attrib_mutex_t<T, null_mutex>;
		using value_type = traits::resource::get_attrib_value_t<T>;
		
		static constexpr int lock_priority = traits::resource::get_attrib_lock_priority_v<T>;
		static constexpr int init_priority = traits::resource::get_attrib_init_priority_v<T>;
		
		using acquire_event_type = ecs::traits::resource::get_attrib_acquire_event_t<T>;
		using release_event_type = ecs::traits::resource::get_attrib_release_event_t<T>;

		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>>;
	};
}

namespace ecs {
    template<ecs::traits::entity_class T>
	struct factory {
		using ecs_tag = ecs::tag::resource;
		using entity_type = T;
		using handle_type = typename entity_traits<T>::handle_type;
		using integral_type = typename handle_type::integral_type;
	
		handle_type inactive = tombstone{ };
		std::vector<handle_type> active;
	};

    template<traits::component_class comp_T> struct manager { 
        using ecs_tag = tag::resource;
		using value_type = std::vector<traits::entity::get_handle_t<traits::component::get_entity_t<comp_T>>>;
    };
    template<traits::component_class comp_T> struct indexer {
        using ecs_tag = ecs::tag::resource;
		using value_type = std::unordered_map<
			typename entity_traits<traits::component::get_entity_t<comp_T>>::handle_type::integral_type, 
			typename entity_traits<traits::component::get_entity_t<comp_T>>::handle_type
		>; // maps entity->index
    };
    template<ecs::traits::component_class T> struct storage {
        using ecs_tag = ecs::tag::resource;
		using value_type = std::vector<traits::component::get_value_t<T>>;
    };
}
