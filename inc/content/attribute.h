#pragma once 
#include "core/traits.h"
#include <vector>
#include <map>

namespace ecs {
    template<traits::component_class T>
    struct manager { 
        using ecs_category = tag::attribute;
        using value_type = std::vector<traits::component::get_handle_t<T>>;
    };
    
    template<traits::component_class T>
    struct indexer {
        using ecs_category = tag::attribute;
        using value_type = std::map<traits::component::get_value_t<T>, std::size_t>;
    };
    
    template<traits::component_class T>
    struct storage {
        using ecs_category = tag::attribute;
        using value_type = std::vector<traits::component::get_value_t<T>>;
    };

    template<traits::entity_class T> 
	struct archive {
		using ecs_category = ecs::tag::attribute;
		using value_type = std::map<std::size_t, std::size_t>;
	};

	template<traits::entity_class T> 
	struct factory {
		using ecs_category = ecs::tag::attribute;
		using value_type = std::map<std::size_t, std::size_t>;

		using create_event = traits::entity::get_create_event_t<T>;
		using destroy_event = traits::entity::get_destroy_event_t<T>;

		void destroy(auto& reg, auto& attrib) {
			if constexpr (!std::is_void_v<destroy_event>) {
				for (auto hnd : attrib) {
					reg.template on<destroy_event>().invoke(hnd);
				}
			}
		}
	};
}
