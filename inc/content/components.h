#pragma once
#include "core/fwd.h"

namespace ecs {
    template<typename T> 
	struct listener : ecs::event_traits<T>::callback_type { 
		using ecs_tag = ecs::tag::component_basictype;
		using entity_type = ecs::event_entity<T>;
	};

}