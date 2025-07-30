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
		
		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>>;
	};
}

namespace ecs {
	

    template<traits::entity_class T>
	struct factory {
		using ecs_tag = tag::resource;
		
		using entity_type = T;
		using handle_type = traits::entity::get_handle_t<entity_type>;

		static constexpr std::size_t page_size = traits::entity::get_page_size_v<entity_type>;
		using value_type = std::vector<std::array<handle_type, page_type>>;
	};

    template<traits::component_class T>
	struct manager { 
		using entity_type = traits::component::get_entity_t<T>;
		using handle_type = traits::entity::get_handle_t<entity_type>;
        
		using ecs_tag = tag::resource;
		using value_type = std::vector<handle_type>;
	};
    
	template<traits::component_class T>
	struct indexer {
        using entity_type = traits::component::get_entity_t<T>;
		using handle_type = traits::entity::get_handle_t<entity_type>;
		
		using ecs_tag = tag::resource;
		using value_type = std::unordered_map<typename handle_type::integral_type, handle_type>;
	};
    
	template<traits::component_class T>
	struct storage {
        using ecs_tag = tag::resource;
		using value_type = std::vector<traits::component::get_value_t<T>>;
	};
}
