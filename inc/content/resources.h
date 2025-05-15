#pragma once 
#include "core/traits.h"
#include <vector>
#include <unordered_map>
#include <shared_mutex>
// resource traits
namespace ecs {
	template<typename T>
	struct resource_traits<T, ecs::tag::resource_unrestricted> {
		static constexpr int lock_priority = traits::resource::get_attrib_lock_priority_v<T>;
		static constexpr int init_priority = traits::resource::get_attrib_init_priority_v<T>;
		using mutex_type = traits::resource::get_attrib_mutex_t<T, null_mutex>;
		using value_type = traits::resource::get_attrib_value_t<T>;

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_restricted> {
		static constexpr int init_priority = traits::resource::get_attrib_init_priority_v<T>;
		static constexpr int lock_priority = traits::resource::get_attrib_lock_priority_v<T>;
		using mutex_type = traits::resource::get_attrib_mutex_t<T, std::shared_mutex>;
		using value_type = traits::resource::get_attrib_value_t<T>;

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_priority> {
		static constexpr int lock_priority = traits::resource::get_attrib_lock_priority_v<T>;
		static constexpr int init_priority = traits::resource::get_attrib_init_priority_v<T>;
		using mutex_type = traits::resource::get_attrib_mutex_t<T, priority_shared_mutex>;
		using value_type = traits::resource::get_attrib_value_t<T>;

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<>;
	};
}

namespace ecs {
    template<ecs::traits::entity_class T>
	struct factory {
		template<ecs::traits::entity_class,typename> friend class generator;
		
		using ecs_tag_type = ecs::tag::resource;
		using entity_type = T;
		using handle_type = typename entity_traits<T>::handle_type;
		using integral_type = typename handle_type::integral_type;
	private:
		handle_type inactive = tombstone{ };
		std::vector<handle_type> active;
	};

    template<ecs::traits::component_class T> struct manager { 
        using ecs_tag_type = ecs::tag::resource;
		using value_type = std::vector<typename entity_traits<traits::component::get_entity_t<T>>::handle_type>;
    };
    template<ecs::traits::component_class T> struct indexer {
        using ecs_tag_type = ecs::tag::resource;
		using value_type = std::unordered_map<std::size_t, typename entity_traits<traits::component::get_entity_t<T>>::handle_type>;
    };
    template<ecs::traits::component_class T> struct storage { 
        using ecs_tag_type = ecs::tag::resource;
		using value_type = std::vector<traits::component::get_value_t<T>>;
    };
}
