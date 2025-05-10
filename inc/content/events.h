#pragma once
#include "core/fwd.h"
#include <functional>

namespace ecs::event {
    template<traits::resource_class T> struct acquire {
		using ecs_tag = ecs::tag::event; // cannot be async
		using callback_type = std::function<void(T&)>;
		static constexpr bool enable_async = false;
	};

	template<traits::resource_class T> struct release {
		using ecs_tag = ecs::tag::event; // cannot be async
		using callback_type = std::function<void(T&)>;
		static constexpr bool enable_async = false;
	};

	template<traits::entity_class T> struct create {
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<void(typename entity_traits<T>::handle_type)>;
	};

	template<traits::entity_class T> struct destroy {
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<void(typename entity_traits<T>::handle_type)>;
	};
	
	template<traits::component_class T> struct init {
	private:
		using component_type = T&;
		using handle_type = typename entity_traits<typename component_traits<T>::entity_type>::handle_type;
		static constexpr bool enable_storage = std::is_same_v<typename component_traits<T>::storage_type, void>;
	public:
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<std::conditional_t<enable_storage, void(handle_type), void(handle_type, T&)>>;
	};
	
	template<traits::component_class T> struct term {
	private:
		using component_type = T&;
		using handle_type = typename entity_traits<typename component_traits<T>::entity_type>::handle_type;
		static constexpr bool enable_storage = std::is_same_v<typename component_traits<T>::storage_type, void>;
	public:
		using ecs_tag = ecs::tag::event;
		using callback_type = std::function<std::conditional_t<enable_storage, void(handle_type), void(handle_type, T&)>>;

	};
}