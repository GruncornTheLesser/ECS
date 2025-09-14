#pragma once 
#include "core/traits.h"
#include <vector>

namespace ecs {
    template<traits::component_class T>
    struct manager { 
        using ecs_category = tag::attribute;
        using value_type = std::vector<traits::component::get_handle_t<T>>;
    };
    
    template<traits::component_class T>
    struct indexer {
        using ecs_category = tag::attribute;
        using value_type = std::vector<traits::component::get_value_t<T>>;
    };
    
    template<traits::component_class T>
    struct storage {
        using ecs_category = tag::attribute;
        using value_type = std::vector<traits::component::get_value_t<T>>;
    };

}
