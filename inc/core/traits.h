#pragma once
#include "core/fwd.h"
#include <shared_mutex>
#include <variant>

// standalone trait value macro declaration
#define DYNAMIC_TRAIT_TYPE(NAME, DEFAULT) 										\
template<typename T, typename D=DEFAULT> 										\
struct get_##NAME { using type = D; }; 											\
template<typename T, typename D> requires requires { typename T::NAME##_type; }	\
struct get_##NAME<T, D> { using type = typename T::NAME##_type; };	            \
template<typename T, typename D=DEFAULT>										\
using get_##NAME##_t = typename get_##NAME<T, D>::type;

#define STATIC_TRAIT_TYPE(NAME, TRAIT)											\
template<typename T>															\
struct get_##NAME { using type = typename TRAIT##_traits<T>::NAME##_type; };	\
template<typename T>															\
using get_##NAME##_t = typename get_##NAME<T>::type;


// standalone trait type macro declaration
#define DYNAMIC_TRAIT_VALUE(TYPE, NAME, DEFAULT) 								\
template<typename T, typename=void> 											\
struct get_##NAME { static constexpr TYPE value = DEFAULT; };		 			\
template<typename T, typename D> requires requires { T::Name; }					\
struct get_##NAME<T, D> { static constexpr TYPE value = T::NAME; };				\
template<typename T>															\
static constexpr TYPE get_##NAME##_v = get_##NAME<T>::value;


namespace ecs::traits::resource {
	DYNAMIC_TRAIT_VALUE(int, lock_priority, 0)
	DYNAMIC_TRAIT_VALUE(int, init_priority, 0)
	DYNAMIC_TRAIT_TYPE(mutex, null_mutex)
	DYNAMIC_TRAIT_TYPE(value, T)
	STATIC_TRAIT_TYPE(cache, resource)
}

namespace ecs::traits::event {
	DYNAMIC_TRAIT_TYPE(callback, void(T&))
	DYNAMIC_TRAIT_TYPE(listener, listener<T>)
	DYNAMIC_TRAIT_VALUE(bool, strict_order, false)
	DYNAMIC_TRAIT_VALUE(bool, enable_async, false)
}

namespace ecs::traits::entity {
	DYNAMIC_TRAIT_TYPE(handle, ECS_DEFAULT_HANDLE)
	DYNAMIC_TRAIT_TYPE(factory, void)
}

namespace ecs::traits::handle {
	STATIC_TRAIT_TYPE(value, handle)
	STATIC_TRAIT_TYPE(version, handle)
	STATIC_TRAIT_TYPE(integral, handle)
}
	
namespace ecs::traits::component {
	DYNAMIC_TRAIT_TYPE(entity, ecs::entity)
	DYNAMIC_TRAIT_TYPE(manager, void)
	DYNAMIC_TRAIT_TYPE(indexer, void)
	DYNAMIC_TRAIT_TYPE(storage, void)
	DYNAMIC_TRAIT_VALUE(bool, enable_deferred, true)
}

// handle traits
namespace ecs {
	template<std::unsigned_integral T>
	struct handle_traits<T> {
		using value_type = typename handle<T, 0>::value_type;
		using version_type = typename handle<T, 0>::version_type;
		using integral_type = typename handle<T, 0>::integral_type;
	};
	template<std::unsigned_integral T, size_t N>
	struct handle_traits<handle<T, N>> {
		using value_type = typename handle<T, N>::value_type;
		using version_type = typename handle<T, N>::version_type;
		using integral_type = typename handle<T, N>::integral_type;
	};
}

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
		using event_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_restricted> {
		static constexpr int init_priority = traits::resource::get_init_priority_v<T>;
		static constexpr int lock_priority = traits::resource::get_lock_priority_v<T>;
		using cache_type = cache<T, std::shared_mutex>;
		
		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T>
	struct resource_traits<T, ecs::tag::resource_priority> {
		static constexpr int lock_priority = traits::resource::get_lock_priority_v<T>;
		static constexpr int init_priority = traits::resource::get_init_priority_v<T>;
		using cache_type = cache<T, priority_shared_mutex>;

		using resource_dependencies = std::tuple<T>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<>;
		using event_dependencies = std::tuple<>;
	};
}

// event traits
namespace ecs {
	template<typename T>
	struct event_traits<T, ecs::tag::event> {
		using callback_type = traits::event::get_callback_t<T>;
		using listener_type = traits::event::get_listener_t<T>;
		using entity_type = component_traits<listener_type>::entity_type;
		static constexpr bool enable_async = traits::event::get_enable_async_v<T>;
		static constexpr bool strict_order = traits::event::get_strict_order_v<T>;
		
		using resource_dependencies = traits::get_resource_dependencies_t<listener_type>;
		using component_dependencies = traits::get_component_dependencies_t<listener_type>;
		using entity_dependencies = traits::get_entity_dependencies_t<listener_type>;
		using event_dependencies = std::tuple<T>;
	};
}

// entity traits
namespace ecs {
	template<typename T>
	struct entity_traits<T, tag::entity_dynamic> {
		using handle_type = ECS_DEFAULT_HANDLE;
		using factory_type = traits::entity::get_factory_t<T, factory<T>>;

		using resource_dependencies = std::tuple<factory_type>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<T>;
		using event_dependencies = std::tuple<>;
	};
	
	template<typename T, std::size_t N>
	struct entity_traits<T, tag::entity_fixed<N>> {
		using handle_type = ECS_DEFAULT_HANDLE;
		using factory_type = traits::entity::get_factory_t<T, factory<T>>;

		using resource_dependencies = std::tuple<factory_type>;
		using component_dependencies = std::tuple<>;
		using entity_dependencies = std::tuple<T>;
		using event_dependencies = std::tuple<>;
	};
}

// component traits
namespace ecs {
	template<typename T>
	struct component_traits<T, tag::component_basictype> {
		using entity_type =  traits::component::get_entity_t<T>;
		using manager_type = traits::component::get_manager_t<T, manager<T>>;
		using indexer_type = traits::component::get_indexer_t<T, indexer<T>>;
		using storage_type = traits::component::get_storage_t<T, storage<T>>;
		using factory_type = typename entity_traits<entity_type>::factory_type;

		using resource_dependencies = std::tuple<manager_type, indexer_type, storage_type, factory_type>;
		using component_dependencies = std::tuple<T>;
		using entity_dependencies = std::tuple<entity_type>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T, typename ... Ts>
	struct component_traits<T, tag::component_archetype<Ts...>> {
	private:
		using primary_type = meta::min_by_t<std::variant<Ts...>, meta::get_type_name>;
	public:
		using entity_type = traits::component::get_entity_t<primary_type>;
		using manager_type = traits::component::get_manager_t<T, manager<primary_type>>;
		using indexer_type = traits::component::get_indexer_t<T, indexer<primary_type>>;
		using storage_type = traits::component::get_storage_t<T, storage<T>>;
		using factory_type = typename entity_traits<entity_type>::factory_type;

		using resource_dependencies = std::tuple<manager_type, indexer_type, typename component_traits<Ts>::storage_type..., factory_type>;
		using component_dependencies = std::tuple<Ts...>;
		using entity_dependencies = std::tuple<entity_type>;
		using event_dependencies = std::tuple<>;
	};

	template<typename T, typename ... Ts>
	struct component_traits<T, tag::component_uniontype<Ts...>> {
		using entity_type = traits::component::get_entity_t<T>;
		using manager_type = traits::component::get_manager_t<T, manager<T>>;
		using indexer_type = traits::component::get_indexer_t<T, indexer<T>>;
		using storage_type = traits::component::get_storage_t<T, meta::sort_by_t<std::variant<Ts...>, meta::get_type_name>>;
		using factory_type = typename entity_traits<entity_type>::factory_type;

		using resource_dependencies = std::tuple<manager_type, indexer_type, storage_type, factory_type>;
		using component_dependencies = std::tuple<Ts...>;
		using entity_dependencies = std::tuple<entity_type>;
		using event_dependencies = std::tuple<>;
	};
}

// concepts definitions
namespace ecs::traits {
	template<typename T, typename> struct is_resource : std::false_type { };
	template<typename T> struct is_resource<T, std::void_t<decltype(resource_traits<std::remove_const_t<T>>{})>> : std::true_type { };

	template<typename T, typename> struct is_event : std::false_type { };
	template<typename T> struct is_event<T, std::void_t<decltype(event_traits<std::remove_const_t<T>>{})>> : std::true_type { };

	template<typename T, typename> struct is_entity : std::false_type { };
	template<typename T> struct is_entity<T, std::void_t<decltype(entity_traits<std::remove_const_t<T>>{})>> : std::true_type { };

	template<typename T, typename> struct is_handle : std::false_type { };
	template<typename T> struct is_handle<T, std::void_t<decltype(handle_traits<std::remove_const_t<T>>{})>> : std::true_type { };

	template<typename T, typename> struct is_component : std::false_type { };
	template<typename T> struct is_component<T, std::void_t<decltype(component_traits<std::remove_const_t<T>>{})>> : std::true_type { };
	
	template<typename T> struct get_traits<T, std::enable_if_t<is_resource_v<T>>> { using type = resource_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_event_v<T>>> { using type = event_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_entity_v<T>>> { using type = entity_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_handle_v<T>>> { using type = handle_traits<T>; };
	template<typename T> struct get_traits<T, std::enable_if_t<is_component_v<T>>> { using type = component_traits<T>; };
}