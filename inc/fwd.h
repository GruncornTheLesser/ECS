#pragma once
#include <type_traits>
#include "id.h"

#ifndef ECS_INDEX_TYPE
#define ECS_INDEX_TYPE uint32_t
#endif

#ifndef ECS_INDEX_WIDTH
#define ECS_INDEX_WIDTH 20
#endif

#ifndef ECS_VERSION_TYPE
#define ECS_VERSION_TYPE uint16_t
#endif

#ifndef ECS_VERSION_WIDTH
#define ECS_VERSION_WIDTH 12
#endif

#ifndef ECS_PAGE_SIZE
#define ECS_PAGE_SIZE 4096
#endif

// fwd.h
namespace ecs {
	// 
	static constexpr std::size_t page_size = ECS_PAGE_SIZE;
	
	// primitives
	using index_t = ECS_INDEX_TYPE;
	using version_t = ECS_VERSION_TYPE;
	struct id;
	struct entity;
	struct indirect;

	// tags
	template<typename T> struct from;
	template<typename...> struct inc;
	template<typename...> struct exc;
	template<typename...> struct any;
	template<auto V>      struct pred;
	template<id>          struct flag;
	template<auto V> requires (std::is_enum_v<decltype(V)> || std::is_same_v<decltype(V), id>) struct state;
	template<typename T, id id_v=std::type_identity<T>{}>  struct resource;
	
	/*
	resource is a tag. wrap a type and id eg resource<character, "player">
	resource doubles as a metaprogram to get resource id and resource type. eg resource<T>::id, resource<T>::type
	*/



	template<typename ... Ts> class registry;
	template<typename type_T, ecs::id key_V=std::type_identity<std::remove_const_t<type_T>>{}> struct cache;
	
    template<typename T> class pool;
	template<typename ... Ts> class view;
	
	
	template<typename ... Ts> struct iter;
	template<typename T, typename Base_T> struct iter_mixin;
	template<typename T=void> struct iter_sentinel;
	
	template<typename ... Ts> struct iter_reference;
	template<typename ... Ts> struct iter_value;
	
	
	// traits
	template<typename T> struct system_traits;
	template<typename T> struct iter_traits;
	template<typename T> struct iter_tag_traits;
	
	/*
	namespace event {
		struct create { };
		struct destroy { };
		template<typename T> struct add { };
		template<typename T> struct remove { };
	}
	*/
}

namespace ecs {
	struct entity {
		constexpr entity(index_t ind = static_cast<index_t>(-1), version_t vers = 0) : index(ind), version(vers) { }
		constexpr bool operator==(const entity& other) const { return index == other.index && version == other.version; }

		index_t index : ECS_INDEX_WIDTH;
		version_t version : ECS_VERSION_WIDTH;
	};

	
	struct indirect {
		constexpr indirect(index_t ind = static_cast<index_t>(-1), version_t vers = 0) : index(ind), version(vers) { }
		
		index_t index : ECS_INDEX_WIDTH;
		version_t version : ECS_VERSION_WIDTH;
	};
}

