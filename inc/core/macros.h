#pragma once

/** 
 * @brief the default type tag used if struct does not declare ecs_tag_type
 */
#ifndef ECS_DEFAULT_TAG
#define ECS_DEFAULT_TAG ecs::tag::component
#endif

/**
 @brief the default component tag if ecs_tag_type equals ecs::tag::component
 */
#ifndef ECS_DEFAULT_COMPONENT_TAG
#define ECS_DEFAULT_COMPONENT_TAG ecs::tag::component_basictype
#endif

/**
 * @brief  the default resource tag if ecs_tag_type equals ecs::tag::resource
 */
#ifndef ECS_DEFAULT_RESOURCE_TAG
#define ECS_DEFAULT_RESOURCE_TAG ecs::tag::resource_restricted
#endif

/**
 * @brief the default entity tag if ecs_tag_type equals ecs::tag::entity
 */
#ifndef ECS_DEFAULT_ENTITY_TAG
#define ECS_DEFAULT_ENTITY_TAG ecs::tag::entity_dynamic
#endif

/**
 * @brief the default event tag if ecs_tag_type equals ecs::tag::event
 */
#ifndef ECS_DEFAULT_EVENT_TAG
#define ECS_DEFAULT_EVENT_TAG ecs::tag::synced_event
#endif

/**
 * @brief the default integral used if entity does not declared handle_integral_type 
 */
#ifndef ECS_DEFAULT_HANDLE_INTEGRAL
#define ECS_DEFAULT_HANDLE_INTEGRAL unsigned int
#endif

/**
 * @brief the default version width used if entity does not declare version_width
 */
#ifndef ECS_DEFAULT_VERSION_WIDTH
#define ECS_DEFAULT_VERSION_WIDTH 12
#endif

/**
 * @brief the default mutex used if resource does not declare mutex_type 
 */
#ifndef ECS_DEFAULT_RESOURCE_MUTEX
#define ECS_DEFAULT_RESOURCE_MUTEX null_mutex
#endif

/**
 * @brief the default component initialization event used if component does not declare init_event_type
 */
#ifndef ECS_DEFAULT_INITIALIZE_EVENT
#define ECS_DEFAULT_INITIALIZE_EVENT void
#endif

/**
 * @brief the default component termination event used if component does not declare term_event_type
 */
#ifndef ECS_DEFAULT_TERMINATE_EVENT
#define ECS_DEFAULT_TERMINATE_EVENT void
#endif

/**
 * @brief macro to ignore commas when passing macro arguments
 */
#define EXPAND(...) __VA_ARGS__

/**
 * @brief declares trait type getter meta functions that retrieve type TRAIT::NAME, 
 * DEFAULT defines the defaulting behaviour if no default type is passed 
 */
#define TRAIT_TYPE(TRAIT, NAME) 												\
template<TRAIT##_class T>														\
struct get_##NAME { using type = typename TRAIT##_traits<T>::NAME##_type; };	\
template<TRAIT##_class T>														\
using get_##NAME##_t = typename get_##NAME<T>::type;

/**
 * @brief declares value getter meta functions that retrieve value TRAIT::NAME 
 * DEFAULT defines the defaulting behaviour if no default value is passed
 */
#define TRAIT_VALUE(TRAIT, TYPE, NAME)											\
template<TRAIT##_class T>														\
struct get_##NAME { static constexpr TYPE value = TRAIT##_traits<T>::NAME; };	\
template<TRAIT##_class T>														\
static constexpr TYPE get_##NAME##_t = get_##NAME<T>::value;

/**
 * @brief declares attribute type getter function that retrieve type T::NAME
 * DEFAULT defines the defaulting behaviour if no default type is passed 
 */
#define ATTRIB_TYPE(NAME, DEFAULT)									        	\
template<typename T, typename D=DEFAULT> 										\
struct get_attrib_##NAME { using type = D; }; 									\
template<typename T, typename D> requires requires { typename T::NAME##_type; }	\
struct get_attrib_##NAME<T, D> { using type = typename T::NAME##_type; };	    \
template<typename T, typename D=DEFAULT>										\
using get_attrib_##NAME##_t = typename get_attrib_##NAME<T, D>::type;

/**
 * @brief declares attribute value getter function that retrieve value T::NAME
 * DEFAULT defines the defaulting behaviour if no default value is passed
 */
#define ATTRIB_VALUE(TYPE, NAME, DEFAULT)				        				\
template<typename T, typename=void> 											\
struct get_attrib_##NAME { static constexpr TYPE value = DEFAULT; };		 	\
template<typename T, typename D> requires requires { T::Name; }					\
struct get_attrib_##NAME<T, D> { static constexpr TYPE value = T::NAME; };		\
template<typename T>															\
static constexpr TYPE get_attrib_##NAME##_v = get_attrib_##NAME<T>::value;

// INFO: because of the order of template instantiation, default behaviour must either 
// be automatically generated from trait attribute, // or assigned individually for each 
// traits type. 