#pragma once
#include "core/traits.h"
#include <functional>

// event traits
namespace ecs {
	template<typename T>
	struct event_traits<T, ecs::tag::event> {
		using callback_type = traits::event::get_attrib_callback_t<T>;
		using listener_type = ecs::traits::event::get_attrib_listener_t<T, ecs::listener<T>>;

		static constexpr bool strict_order = traits::event::get_attrib_strict_order_v<T>;
		static constexpr bool enable_fire_once = traits::event::get_attrib_enable_fire_once_v<T>;

		using dependency_set = util::append_t<ecs::traits::dependencies::get_attrib_dependency_set_t<T>, listener_type>;
	};
}

namespace ecs::event {
	template<traits::entity_class T> struct create {
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<void(traits::entity::get_handle_t<T>)>;
	};

	template<traits::entity_class T> struct destroy {
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<void(traits::entity::get_handle_t<T>)>;
	};
	
	template<traits::component_class T> struct sync {
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<void()>;
	};

	template<traits::component_class comp_T> struct initialize {
	private:
		static constexpr bool enable_storage = std::is_void_v<traits::component::get_value_t<comp_T>>;
		using value_type = util::eval_try_t<comp_T, traits::component::get_value>;
		using handle_type = ecs::traits::entity::get_handle_t<ecs::traits::component::get_entity_t<comp_T>>;
	public:
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<std::conditional_t<enable_storage, void(handle_type), void(handle_type, value_type&)>>;
	};
	
	template<traits::component_class comp_T> struct terminate {
	private:
		static constexpr bool enable_storage = std::is_void_v<traits::component::get_value_t<comp_T>>;
		using value_type = util::eval_try_t<comp_T, traits::component::get_value>;
		using handle_type = ecs::traits::entity::get_handle_t<ecs::traits::component::get_entity_t<comp_T>>;
	public:
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<std::conditional_t<enable_storage, void(handle_type), void(handle_type, value_type&)>>;

	};
}