#pragma once

/** 
 * @brief the default type tag used if struct does not declare ecs_tag
 */
#ifndef ECS_DEFAULT_TAG
#define ECS_DEFAULT_TAG ecs::tag::component
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
 * @brief the default entity used if component does not declare entity_type 
 */
 #ifndef ECS_DEFAULT_ENTITY
 #define ECS_DEFAULT_ENTITY ecs::entity
 #endif

/**
 * @brief the default acquire event used if resource does not declare acquire_event 
 */
 #ifndef ECS_DEFAULT_ACQUIRE_EVENT
 #define ECS_DEFAULT_ACQUIRE_EVENT void
 #endif
  
/**
 * @brief the default release event used if resource does not declare release_event
 */
#ifndef ECS_DEFAULT_RELEASE_EVENT
#define ECS_DEFAULT_RELEASE_EVENT void
#endif
 
/**
 * @brief the default initialize event used if component does not declare init_event 
 */
#ifndef ECS_DEFAULT_INITIALIZE_EVENT
#define ECS_DEFAULT_INITIALIZE_EVENT void
#endif
 
/**
 * @brief the default terminate event used if resource does not declare term_event 
 */
#ifndef ECS_DEFAULT_TERMINATE_EVENT
#define ECS_DEFAULT_TERMINATE_EVENT void
#endif
 
/**
 * @brief the default create event used if entity does not declare create_event 
 */
#ifndef ECS_DEFAULT_CREATE_EVENT
#define ECS_DEFAULT_CREATE_EVENT void
#endif
 
/**
 * @brief the default destroy event used if entity does not declare destroy_event
 */
#ifndef ECS_DEFAULT_DESTROY_EVENT
#define ECS_DEFAULT_DESTROY_EVENT void
#endif
 
 
/**
 * @brief macro to ignore commas when passing macro arguments
 */
#define EXPAND(...) __VA_ARGS__

/**
 * @brief declares trait type getter meta functions that retrieve type TRAIT::NAME, 
 * DEFAULT defines the defaulting behaviour if no default type is passed 
 */
#define TRAIT_TYPE(NAME, ALIAS, TRAIT)											\
template<TRAIT##_class T>														\
struct get_##NAME { using type = typename TRAIT##_traits<T>::ALIAS; };	        \
template<TRAIT##_class T>														\
using get_##NAME##_t = typename get_##NAME<T>::type;

/**
 * @brief declares value getter meta functions that retrieve value TRAIT::NAME 
 * DEFAULT defines the defaulting behaviour if no default value is passed
 */
#define TRAIT_VALUE(TYPE, NAME, ALIAS, TRAIT)									\
template<TRAIT##_class T>														\
struct get_##NAME { static constexpr TYPE value = TRAIT##_traits<T>::ALIAS; };	\
template<TRAIT##_class T>														\
static constexpr TYPE get_##NAME##_v = get_##NAME<T>::value;

/**
 * @brief declares attribute type getter function that retrieve type T::NAME
 * DEFAULT defines the defaulting behaviour if no default type is passed 
 */
#define ATTRIB_TYPE(NAME, ALIAS, DEFAULT)							        	\
template<typename T, typename D=DEFAULT> 										\
struct get_attrib_##NAME { using type = D; }; 									\
template<typename T, typename D> requires requires { typename T::ALIAS; }	    \
struct get_attrib_##NAME<T, D> { using type = typename T::ALIAS; }; 	        \
template<typename T, typename D=DEFAULT>										\
using get_attrib_##NAME##_t = typename get_attrib_##NAME<T, D>::type;

/**
 * @brief declares attribute value getter function that retrieve value T::NAME
 * DEFAULT defines the defaulting behaviour if no default value is passed
 */
#define ATTRIB_VALUE(TYPE, NAME, ALIAS, DEFAULT)		        				\
template<typename T, typename=void> 											\
struct get_attrib_##NAME { static constexpr TYPE value = DEFAULT; };		 	\
template<typename T, typename D> requires requires { T::ALIAS; }				\
struct get_attrib_##NAME<T, D> { static constexpr TYPE value = T::ALIAS; };		\
template<typename T>															\
static constexpr TYPE get_attrib_##NAME##_v = get_attrib_##NAME<T>::value;

// INFO: because of the order of template instantiation, default behaviour must either 
// be automatically generated from trait attribute, // or assigned individually for each 
// traits type. 