#pragma once 
#include "core/traits.h"
#include <vector>
#include <unordered_map>
#include <shared_mutex>
// resource traits
namespace ecs {
	template<typename T>
	struct resource_traits<T, ecs::tag::resource_unrestricted> {
		static constexpr int lock_priority = traits::resource::get_lock_priority_v<T>;
		static constexpr int init_priority = traits::resource::get_init_priority_v<T>;
		using cache_type = cache<T, null_mutex>;

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_restricted> {
		static constexpr int init_priority = traits::resource::get_init_priority_v<T>;
		static constexpr int lock_priority = traits::resource::get_lock_priority_v<T>;
		using cache_type = cache<T, std::shared_mutex>;
		
		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_priority> {
		static constexpr int lock_priority = traits::resource::get_lock_priority_v<T>;
		static constexpr int init_priority = traits::resource::get_init_priority_v<T>;
		using cache_type = cache<T, priority_shared_mutex>;

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

    template<ecs::traits::component_class T> struct manager
        : std::vector<typename entity_traits<typename component_traits<T>::entity_type>::handle_type> { 
        using ecs_tag_type = ecs::tag::resource;
    };
    template<ecs::traits::component_class T> struct indexer : std::unordered_map<std::size_t, typename entity_traits<typename component_traits<T>::entity_type>::handle_type> {
        using ecs_tag_type = ecs::tag::resource;
    };
    template<ecs::traits::component_class T> struct storage : std::vector<T> { 
        using ecs_tag_type = ecs::tag::resource;
    };
}
