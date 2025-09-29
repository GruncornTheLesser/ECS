#pragma once

#include "core/fwd.h"
#include <util.h>
#include <cstdint>

#ifndef ECS_RECURSIVE_DEPENDENCY
#define ECS_RECURSIVE_DEPENDENCY true
#endif

#ifndef ECS_DYNAMIC_REGISTRY
#define ECS_DYNAMIC_REGISTRY true
#endif

/** the default type tag used if struct does not declare ecs_category */
#ifndef ECS_DEFAULT_TAG
#define ECS_DEFAULT_TAG ecs::tag::component
#endif

/* the default integral used if entity does not declared handle_integral_type  */
#ifndef ECS_DEFAULT_HANDLE_INTEGRAL
#define ECS_DEFAULT_HANDLE_INTEGRAL uint32_t
#endif

#ifndef ECS_DEFAULT_HANDLE_VERSION_WIDTH
#define ECS_DEFAULT_HANDLE_VERSION_WIDTH 12
#endif

#ifndef ECS_DEFAULT_PAGE_SIZE
#define ECS_DEFAULT_PAGE_SIZE 4096
#endif

/* the default mutex used if attribute does not declare mutex_type  */
#ifndef ECS_DEFAULT_MUTEX
#define ECS_DEFAULT_MUTEX ecs::priority_mutex
#endif

/* the default entity used if component does not declare entity_type  */
 #ifndef ECS_DEFAULT_ENTITY
 #define ECS_DEFAULT_ENTITY ecs::entity
 #endif

/* the default addialize event used if component does not declare init_event  */
#ifndef ECS_DEFAULT_INITIALIZE_EVENT
#define ECS_DEFAULT_INITIALIZE_EVENT ecs::event::initialize<T>
#endif
 
/* the default terminate event used if attribute does not declare term_event  */
#ifndef ECS_DEFAULT_TERMINATE_EVENT  
#define ECS_DEFAULT_TERMINATE_EVENT ecs::event::terminate<T>
#endif
 
/* the default create event used if entity does not declare create_event  */
#ifndef ECS_DEFAULT_CREATE_EVENT
#define ECS_DEFAULT_CREATE_EVENT ecs::event::create<T>
#endif
 
/* the default destroy event used if entity does not declare destroy_event */
#ifndef ECS_DEFAULT_DESTROY_EVENT
#define ECS_DEFAULT_DESTROY_EVENT ecs::event::destroy<T>
#endif

#ifndef ECS_DEFAULT_ACQUIRE_EVENT
#define ECS_DEFAULT_ACQUIRE_EVENT ecs::event::acquire<T>
#endif

#ifndef ECS_DEFAULT_RELEASE_EVENT
#define ECS_DEFAULT_RELEASE_EVENT ecs::event::release<T>
#endif

/* macro to ignore commas when passing macro arguments */
#define EXPAND(...) __VA_ARGS__

/**
 * declares trait type getter meta functions that retrieve type TRAIT::NAME, 
 * DEFAULT defines the default type none declared default type is passed 
 */
#define TRAIT_TYPE(NAME, ALIAS, TRAIT)											\
template<typename T>															\
struct get_##NAME { using type = typename TRAIT##_traits<T>::ALIAS; };			\
template<typename T>															\
using get_##NAME##_t = typename get_##NAME<T>::type;

/**
 * declares value getter meta functions that retrieve value TRAIT::NAME 
 * DEFAULT defines the default value none declared 
 */
#define TRAIT_VALUE(TYPE, NAME, ALIAS, TRAIT)									\
template<typename T>															\
struct get_##NAME { static constexpr TYPE value = TRAIT##_traits<T>::ALIAS; };	\
template<typename T>															\
static constexpr TYPE get_##NAME##_v = get_##NAME<T>::value;

/**
 * declares attribute type getter function that retrieve type T::NAME
 * DEFAULT defines the default type none declared
 */
#define TRAIT_ATTRIB_TYPE(NAME, ALIAS)											\
template<typename T, typename D> 												\
struct get_trait_##NAME { using type = D; }; 									\
template<typename T, typename D> requires requires { typename T::ALIAS; }		\
struct get_trait_##NAME<T, D> { using type = typename T::ALIAS; }; 				\
template<typename T, typename D>												\
using get_trait_##NAME##_t = typename get_trait_##NAME<T, D>::type;

/**
 * declares attribute value getter function that retrieve value T::NAME
 * DEFAULT defines the default value none declared 
 */
#define TRAIT_ATTRIB_VALUE(TYPE, NAME, ALIAS)									\
template<typename T, TYPE D>		 											\
struct get_trait_##NAME { static constexpr TYPE value = D; };					\
template<typename T, TYPE D> requires requires { T::ALIAS; }					\
struct get_trait_##NAME<T, D> { static constexpr TYPE value = T::ALIAS; };		\
template<typename T, TYPE D>													\
static constexpr TYPE get_trait_##NAME##_v = get_trait_##NAME<T, D>::value;



namespace ecs::traits {
	TRAIT_ATTRIB_TYPE(ecs_category, ecs_category)
	
	template<typename T> struct is_attribute : std::disjunction<std::is_same<get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>, tag::attribute>, std::is_base_of<tag::attribute, get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>>> { };
	template<typename T> struct is_entity : std::disjunction<std::is_same<get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>, tag::entity>, std::is_base_of<tag::entity, get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>>> { };
	template<typename T> struct is_component : std::disjunction<std::is_same<get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>, tag::component>, std::is_base_of<tag::component, get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>>> { };
	template<typename T> struct is_event : std::disjunction<std::is_same<get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>, tag::event>, std::is_base_of<tag::event, get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>>> { };
}

namespace ecs {
	template<typename T, typename=traits::get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>> struct attribute_traits;
	template<typename T, typename=traits::get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>> struct entity_traits;
	template<typename T, typename=traits::get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>> struct component_traits;
	template<typename T, typename=traits::get_trait_ecs_category_t<T, ECS_DEFAULT_TAG>> struct event_traits;

	template<typename T> struct attribute_traits<const T, tag::attribute> : attribute_traits<T, tag::attribute> { };
	template<typename T> struct entity_traits<const T, tag::entity> : entity_traits<T, tag::entity> { };
	template<typename T> struct component_traits<const T, tag::component> : component_traits<T, tag::component> { };
	template<typename T> struct event_traits<const T, tag::event> : event_traits<T, tag::event> { };
}

namespace ecs::traits::attribute {
	/* determines the order in which registery::lock() acquires the mutex of each attribute. */
	TRAIT_ATTRIB_VALUE(int, lock_priority, lock_priority)
	TRAIT_VALUE(int, lock_priority, lock_priority, attribute)
	
	/* determines the order in which registry::registry() initializes each attribute. */
	TRAIT_ATTRIB_VALUE(int, init_priority, init_priority)
	TRAIT_VALUE(int, init_priority, init_priority, attribute)
	
	/* the type used to lock a attribute. */
	TRAIT_ATTRIB_TYPE(mutex, mutex_type)
	TRAIT_TYPE(mutex, mutex_type, attribute)

	/* the value of the attribute allows a decoupling of the attribute id type and the value return type. */
	TRAIT_ATTRIB_TYPE(value, value_type)
	TRAIT_TYPE(value, value_type, attribute)
		
	/* the event fired when a component is locked */
	TRAIT_ATTRIB_TYPE(acquire_event, acquire_event)
	TRAIT_TYPE(acquire_event, acquire_event, attribute)
	
	/* the event fired when a component is released */
	TRAIT_ATTRIB_TYPE(release_event, release_event)
	TRAIT_TYPE(release_event, release_event, attribute)
}

namespace ecs::traits::entity {
	/* the handle type is the primitive key representing an entity. */
	TRAIT_TYPE(handle, handle_type, entity)
	TRAIT_ATTRIB_TYPE(integral, integral_type)
	TRAIT_ATTRIB_VALUE(std::size_t, version_width, version_width)
	

	/* the event fired when an entity is created */
	TRAIT_TYPE(create_event, create_event, entity)
	TRAIT_ATTRIB_TYPE(create_event, create_event)
	
	/* the event fired when an entity is destroyed */
	TRAIT_TYPE(destroy_event, destroy_event, entity)
	TRAIT_ATTRIB_TYPE(destroy_event, destroy_event)

	/* the factory is an attribute to create new handle indices. */
	TRAIT_TYPE(factory, factory_type, entity)
	TRAIT_ATTRIB_TYPE(factory, factory_type)

	/* the archive is an attribute to store destroyed handles. */
	TRAIT_TYPE(archive, archive_type, entity)
	TRAIT_ATTRIB_TYPE(archive, archive_type)
}

namespace ecs::traits::component {
	/* component value type decouples the component value from the component id type */
	TRAIT_TYPE(value, value_type, component)
	TRAIT_ATTRIB_TYPE(value, value_type)

	/* component entity type determines the entity table the component belongs to */
	TRAIT_TYPE(entity, entity_type, component)
	TRAIT_ATTRIB_TYPE(entity, entity_type)
	
	/* the event fired when a component is initialized */
	TRAIT_TYPE(initialize_event, initialize_event, component)
	TRAIT_ATTRIB_TYPE(initialize_event, initialize_event)
	
	/* the event fired when a component is terminated */
	TRAIT_TYPE(terminate_event, terminate_event, component)
	TRAIT_ATTRIB_TYPE(terminate_event, terminate_event)

	template<traits::component_class T>
	struct get_handle;
	template<traits::component_class T> requires (!std::is_void_v<get_entity_t<T>>)
	struct get_handle<T> { using type = traits::entity::get_handle_t<get_entity_t<T>>; };
	template<traits::component_class T> requires (std::is_void_v<get_entity_t<T>> && requires { typename T::handle_type; })
	struct get_handle<T> { using type = typename T::handle_type; };
	template<traits::component_class T> requires (std::is_void_v<get_entity_t<T>> && !requires { typename T::handle_type; })
	struct get_handle<T> { using type = std::size_t; };
	template<traits::component_class T> 
	using get_handle_t = typename get_handle<T>::type;

	/* the manager attribute stores the sequence of entity handles. */
	TRAIT_TYPE(manager, manager_type, component)
	TRAIT_ATTRIB_TYPE(manager, manager_type)
	/* the indexer attribute provides a lookup to the index of the handle */
	TRAIT_TYPE(indexer, indexer_type, component)
	TRAIT_ATTRIB_TYPE(indexer, indexer_type)
	/* the storage attribute stores the component value_type corresponding to the entity. */
	TRAIT_TYPE(storage, storage_type, component)
	TRAIT_ATTRIB_TYPE(storage, storage_type)

	/* the storage attribute stores the component value_type corresponding to the entity. */
	TRAIT_VALUE(std::size_t, page_size, page_size, component)
	TRAIT_ATTRIB_VALUE(std::size_t, page_size, page_size)
}

namespace ecs::traits::event {
	/* the callback type decaring the function signature of the event */
	TRAIT_ATTRIB_TYPE(callback, callback_type)
	TRAIT_TYPE(callback, callback_type, event)

	/* the listener type is the function wrapper that fires each individual subscriber to an event */
	TRAIT_ATTRIB_TYPE(listener, listener_type)
	TRAIT_TYPE(listener, listener_type, event)

	/* the sequence policy determines the order of the executed listeners. */
	TRAIT_ATTRIB_TYPE(sequence_policy, sequence_policy)
	TRAIT_TYPE(sequence_policy, sequence_policy, event)
}

// dependencies
namespace ecs::traits::dependencies {
	TRAIT_ATTRIB_TYPE(dependencies, dependency_set)
	
	template<typename T>
	struct get_dependency_set;
	template<typename T>
	using get_dependency_set_t = typename get_dependency_set<T>::type;

	template<traits::attribute_class T> struct get_dependency_set<T> { using type = typename attribute_traits<T>::dependency_set; };
	template<traits::entity_class T> struct get_dependency_set<T> { using type = typename entity_traits<T>::dependency_set; };
	template<traits::component_class T> struct get_dependency_set<T> { using type = typename component_traits<T>::dependency_set; };
	template<traits::event_class T> struct get_dependency_set<T> { using type = typename event_traits<T>::dependency_set; };

	namespace details {	
		template<typename Tup, typename Out=std::tuple<>>
		struct recurse_dependency_set {
			using set = util::filter_t<Tup, util::pred::disj_<util::pred::element_of_<Out, util::cmp::is_ignore_const_same>::template type, std::is_void>::template inv>;
			using dep = util::eval_t<Tup, util::eval_each_<util::propagate_const_each_<dependencies::get_dependency_set>::template type>::template type, util::concat, util::unique_<>::template type, util::filter_<std::is_void>::template inv>;
			
			static constexpr auto get_dependencies() {
				if constexpr (!ECS_RECURSIVE_DEPENDENCY) {
					return std::type_identity<dep>{};
				} else if constexpr (std::is_same_v<std::tuple<>, set>) {
					return std::type_identity<Out>{};
				} else {
					return std::type_identity<typename dependencies::details::recurse_dependency_set<dep, util::concat_t<std::tuple<Out, set>>>::type>{};
				}
			}
		public:
			using type = decltype(get_dependencies())::type;
		};
		template<typename ... Ts> using recurse_dependency_set_t = typename recurse_dependency_set<Ts...>::type;
	}
	template<typename ... Ts> struct get_attribute_set { using type = util::filter_t<details::recurse_dependency_set_t<std::tuple<Ts...>>, is_attribute>; };
	template<typename ... Ts> using get_attribute_set_t = typename get_attribute_set<Ts...>::type;

	template<typename ... Ts> struct get_entity_set { using type = util::filter_t<details::recurse_dependency_set_t<std::tuple<Ts...>>, is_entity>; };
	template<typename ... Ts> using get_entity_set_t = typename get_entity_set<Ts...>::type;

	template<typename ... Ts> struct get_component_set { using type = util::filter_t<details::recurse_dependency_set_t<std::tuple<Ts...>>, is_component>; };
	template<typename ... Ts> using get_component_set_t = typename get_component_set<Ts...>::type;

	template<typename ... Ts> struct get_event_set { using type = util::filter_t<details::recurse_dependency_set_t<std::tuple<Ts...>>, is_event>; };
	template<typename ... Ts> using get_event_set_t = typename get_event_set<Ts...>::type;
} // ecs::traits::dependencies


#undef EXPAND
#undef TRAIT_TYPE
#undef TRAIT_VALUE
#undef TRAIT_FUNC
#undef TRAIT_ATTRIB_TYPE
#undef TRAIT_ATTRIB_VALUE

namespace ecs::tag {
	struct attribute { }; // default
	struct entity { };    // default
	struct component { }; // default
	struct event { };     // default

	struct flag : component { 
		using value_type = void;
		using initialize_event = void;
		using terminate_event = void;
		using storage_type = void;
		using manager_type = void;
	};
};

namespace ecs {	
	template<typename T, typename tag_T>
	struct attribute_traits {
		using value_type = traits::attribute::get_trait_value_t<T, traits::attribute::get_trait_value_t<tag_T, T>>;
		using mutex_type = traits::attribute::get_trait_mutex_t<T, traits::attribute::get_trait_mutex_t<tag_T, ECS_DEFAULT_MUTEX>>;

		using acquire_event = traits::attribute::get_trait_acquire_event_t<T, traits::attribute::get_trait_acquire_event_t<tag_T, ECS_DEFAULT_ACQUIRE_EVENT>>;
		using release_event = traits::attribute::get_trait_release_event_t<T, traits::attribute::get_trait_release_event_t<tag_T, ECS_DEFAULT_RELEASE_EVENT>>;
		
		static constexpr int lock_priority = traits::attribute::get_trait_lock_priority_v<T, traits::attribute::get_trait_lock_priority_v<tag_T, 0>>;
		static constexpr int init_priority = traits::attribute::get_trait_init_priority_v<T, traits::attribute::get_trait_init_priority_v<tag_T, 0>>;

		using dependency_set = util::push_back_t<traits::dependencies::get_trait_dependencies_t<T, traits::dependencies::get_trait_dependencies_t<tag_T, std::tuple<>>>, acquire_event, release_event>;
	};

	template<typename T, typename tag_T>
	struct entity_traits {
		using handle_type = ecs::handle<traits::entity::get_trait_integral_t<T, uint32_t>, traits::entity::get_trait_version_width_v<T, 12>>;
		
		using create_event = traits::entity::get_trait_create_event_t<T, traits::entity::get_trait_create_event_t<tag_T, ECS_DEFAULT_CREATE_EVENT>>;
		using destroy_event = traits::entity::get_trait_destroy_event_t<T, traits::entity::get_trait_destroy_event_t<tag_T, ECS_DEFAULT_DESTROY_EVENT>>;
		
		using factory_type = traits::entity::get_trait_factory_t<T, traits::entity::get_trait_factory_t<tag_T, factory<T>>>;
		
		using dependency_set = util::push_back_t<traits::dependencies::get_trait_dependencies_t<T, traits::dependencies::get_trait_dependencies_t<tag_T, std::tuple<>>>, factory_type, create_event, destroy_event>;
	};

	template<typename T, typename tag_T>
	struct component_traits {
		using value_type = traits::attribute::get_trait_value_t<T, traits::attribute::get_trait_value_t<tag_T, T>>;
		using entity_type = traits::component::get_trait_entity_t<T, traits::component::get_trait_entity_t<tag_T, ECS_DEFAULT_ENTITY>>;

		using initialize_event = traits::component::get_trait_initialize_event_t<T, traits::component::get_trait_initialize_event_t<tag_T, ECS_DEFAULT_INITIALIZE_EVENT>>;
		using terminate_event = traits::component::get_trait_terminate_event_t<T, traits::component::get_trait_terminate_event_t<tag_T, ECS_DEFAULT_TERMINATE_EVENT>>;

		using manager_type = traits::component::get_trait_manager_t<T, traits::component::get_trait_manager_t<tag_T, manager<T>>>;
		using indexer_type = traits::component::get_trait_indexer_t<T, traits::component::get_trait_indexer_t<tag_T, indexer<T>>>;
		using storage_type = traits::component::get_trait_storage_t<T, traits::component::get_trait_storage_t<tag_T, std::conditional_t<(std::is_empty_v<value_type> || std::is_void_v<value_type>), void, storage<T>>>>;

		static constexpr std::size_t page_size = traits::component::get_trait_page_size_v<T, traits::component::get_trait_page_size_v<tag_T, ECS_DEFAULT_PAGE_SIZE>>;

		using dependency_set = util::push_back_t<traits::dependencies::get_trait_dependencies_t<T, traits::dependencies::get_trait_dependencies_t<tag_T, std::tuple<>>>, initialize_event, terminate_event, entity_type, manager_type, indexer_type, storage_type>;
	};

	template<typename T, typename tag_T>
	struct event_traits {
		using callback_type = traits::event::get_trait_callback_t<T, traits::event::get_trait_callback_t<tag_T, void(T&)>>;
		using listener_type = traits::event::get_trait_listener_t<T, traits::event::get_trait_listener_t<tag_T, listener<T>>>;

		using dependency_set = util::push_back_t<traits::dependencies::get_trait_dependencies_t<T, traits::dependencies::get_trait_dependencies_t<tag_T, std::tuple<>>>, listener<T>>;
	};
}