#pragma once
#include "core/traits.h"
#include <functional>

// event traits
namespace ecs {
	template<typename T>
	struct event_traits<T, ecs::tag::synced_event> {
		using listener_type = ecs::listener<T>;
		using callback_type = traits::event::get_callback_t<T>;
		using entity_type = component_traits<listener_type>::entity_type;
		static constexpr bool enable_async = traits::event::get_enable_async_v<T>;
		static constexpr bool strict_order = traits::event::get_strict_order_v<T>;
		
		using resource_dependencies = traits::get_resource_dependencies_t<listener_type>;
		using component_dependencies = traits::get_component_dependencies_t<listener_type>;
		using entity_dependencies = traits::get_entity_dependencies_t<listener_type>;
	};
}



namespace ecs::event {
    template<traits::resource_class T> struct acquire {
		using ecs_tag_type = ecs::tag::event; // cannot be async
		using callback_type = std::function<void(T&)>;
		static constexpr bool enable_async = false;
	};

	template<traits::resource_class T> struct release {
		using ecs_tag_type = ecs::tag::event; // cannot be async
		using callback_type = std::function<void(T&)>;
		static constexpr bool enable_async = false;
	};

	template<traits::entity_class T> struct create {
		using ecs_tag_type = ecs::tag::event;
		using callback_type = std::function<void(typename entity_traits<T>::handle_type)>;
	};

	template<traits::entity_class T> struct destroy {
		using ecs_tag_type = ecs::tag::event;
		using callback_type = std::function<void(typename entity_traits<T>::handle_type)>;
	};
	
	template<traits::component_class T> struct init {
	private:
		using component_type = T&;
		using handle_type = typename entity_traits<typename component_traits<T>::entity_type>::handle_type;
		static constexpr bool enable_storage = std::is_same_v<typename component_traits<T>::storage_type, void>;
	public:
		using ecs_tag_type = ecs::tag::event;
		using callback_type = std::function<std::conditional_t<enable_storage, void(handle_type), void(handle_type, T&)>>;
	};
	
	template<traits::component_class T> struct term {
	private:
		using component_type = T&;
		using handle_type = typename entity_traits<typename component_traits<T>::entity_type>::handle_type;
		static constexpr bool enable_storage = std::is_same_v<typename component_traits<T>::storage_type, void>;
	public:
		using ecs_tag_type = ecs::tag::event;
		using callback_type = std::function<std::conditional_t<enable_storage, void(handle_type), void(handle_type, T&)>>;

	};
}