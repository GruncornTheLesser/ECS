#pragma once
#include "core/traits.h"

namespace ecs::event {
	template<traits::entity_class T, typename reg_T>
	struct create {
		using ecs_category = tag::event;
		
		template<typename reg_U> 
		using rebind_registry = create<T, reg_U>;
		
		using callback_type = void(reg_T&, traits::entity::get_handle_t<T>);
	};

	template<traits::entity_class T, typename reg_T>
	struct destroy {
		using ecs_category = tag::event;

		template<typename reg_U> 
		using rebind_registry = destroy<T, reg_U>;
		
		using callback_type = void(reg_T&, traits::entity::get_handle_t<T>);
	};
	
	template<traits::component_class T, typename reg_T>
	struct initialize {
	private:
		using value_type = traits::component::get_value_t<T>;
		using handle_type = traits::component::get_handle_t<T>;
	public:
		using ecs_category = tag::event;

		template<typename reg_U> 
		using rebind_registry = initialize<T, reg_U>;
		
		using callback_type = typename decltype([] {
			if constexpr (std::is_void_v<value_type> || std::is_empty_v<value_type>) 
				return std::type_identity<void(reg_T&, handle_type)>{};
			else 
				return std::type_identity<void(reg_T&, handle_type, value_type&)>{}; 
		}())::type;
	};
	
	template<traits::component_class T, typename reg_T>
	struct terminate {
	private:
		using value_type = traits::component::get_value_t<T>;
		using handle_type = traits::component::get_handle_t<T>;
	public:
		using ecs_category = tag::event;

		template<typename reg_U> 
		using rebind_registry = terminate<T, reg_U>;

		using callback_type = typename decltype([] {
			if constexpr (std::is_void_v<value_type> || std::is_empty_v<value_type>) 
				return std::type_identity<void(reg_T&, handle_type)>{};
			else 
				return std::type_identity<void(reg_T&, handle_type, value_type&)>{}; 
		}())::type;
	};

	template<traits::attribute_class T, typename reg_T>
	struct acquire {
		using ecs_category = tag::event;

		template<typename reg_U> 
		using rebind_registry = acquire<T, reg_U>;

		using callback_type = void(reg_T&, traits::attribute::get_value_t<T>&);
	};
		
	template<traits::attribute_class T, typename reg_T>
	struct release {
		using ecs_category = tag::event;

		template<typename reg_U> 
		using rebind_registry = release<T, reg_U>;

		using callback_type = void(reg_T&, traits::attribute::get_value_t<T>&);
	};
}