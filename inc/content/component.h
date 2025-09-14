#pragma once
#include "core/traits.h"

namespace ecs {
	template<ecs::traits::event_class T> 
	struct listener {
		using ecs_category = ecs::tag::component;
		
		using value_type = traits::event::get_callback_t<T>;
		using entity_type = ecs::event_entity<T>;
		using initialize_event = void;
		using terminate_event = void;
	};
}