#pragma once
#include "core/traits.h"
#include <functional>

namespace ecs::event {
	template<traits::entity_class T>
	struct create {
		using ecs_category = tag::event;
		using callback_type = std::function<void(traits::entity::get_handle_t<T>)>;
	};

	template<traits::entity_class T>
	struct destroy {
		using ecs_category = tag::event;
		using callback_type = std::function<void(traits::entity::get_handle_t<T>)>;
	};
	
	template<traits::component_class comp_T>
	struct initialize {
	private:
		using value_type = traits::component::get_value_t<comp_T>;
		using handle_type = traits::component::get_handle_t<comp_T>;
	public:
		using ecs_category = tag::event;
		using callback_type = std::function<typename decltype([]{ 
			if constexpr (std::is_void_v<value_type> || std::is_empty_v<value_type>) return std::type_identity<void(handle_type)>{};
			else return std::type_identity<void(handle_type, value_type&)>{}; 
		}())::type>;
	};
	
	template<traits::component_class comp_T>
	struct terminate {
	private:
		using value_type = traits::component::get_value_t<comp_T>;
		using handle_type = traits::component::get_handle_t<comp_T>;
	public:
		using ecs_category = tag::event;
		using callback_type = std::function<typename decltype([]{ 
			if constexpr (std::is_void_v<value_type> || std::is_empty_v<value_type>) return std::type_identity<void(handle_type)>{};
			else return std::type_identity<void(handle_type, value_type&)>{}; 
		}())::type>;
	};

	template<traits::attribute_class T>
	struct acquire {
		using ecs_category = tag::event;
		using callback_type = std::function<void(traits::attribute::get_value_t<T>&)>;
	};
		
	template<traits::attribute_class T>
	struct release {
		using ecs_category = tag::event;
		using callback_type = std::function<void(traits::component::get_value_t<T>&)>;
	};
}