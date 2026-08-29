#pragma once
#include <type_traits>
#include "label.h"

#ifndef ECS_HANDLE_INTEGRAL
#define ECS_HANDLE_INTEGRAL uint32_t
#endif

#ifndef ECS_HANDLE_FACTOR
#define ECS_HANDLE_FACTOR 0.625
#endif

#ifndef ECS_PAGE_SIZE
#define ECS_PAGE_SIZE 4096
#endif
 
namespace ecs {
	static constexpr std::size_t page_size = ECS_PAGE_SIZE;
	
	// primitives
	struct label;
	struct entity;
	struct indirect;

	// registry cache
	template<typename T, label=std::type_identity<T>{}> struct res { };
	
	// view tags
	template<typename T> struct from;
	template<typename...> struct inc { };
	template<typename...> struct exc { };
	template<typename...> struct any { };
	template<label>       struct flag { };
	template<auto V> requires (std::is_enum_v<decltype(V)> || std::is_same_v<decltype(V), label>) struct state { };
	
	template<typename ... Ts> class res_cache;
	template<typename ... Ts> class registry;
	
    template<typename T>      class pool;
	template<typename ... Ts> class view;
	
	template<typename ... Ts> struct iter;
	template<typename ... Ts> struct iter_reference;
	template<typename ... Ts> struct iter_value;
	template<typename=void> struct iter_sentinel;
	template<typename T, typename Base_T> struct iter_mixin;
	
	
	namespace tag {
		enum attributes {
			primary = 0x01, // the iter mixin can be used as the primary iterator
			updated = 0x02, // the iter mixin is incremented and decremented
			ordered = 0x04, // the iter mixin can be used for iter comparisons
			guarded = 0x08, // the iter mixin guards increment and decrement, converts the iter to a bidirectional iter
			compose = 0x10, // the iter mixin composes a ref and val mixin
			exposed = 0x20, // the ref and val mixin are exposed in the structured binding
			compare = 0x40, // the ref and val mixin can be used in ref/val comparisons. only a single mixin is compared
		};
	}
	
	// traits
	template<typename T> struct res_traits;
	template<typename T> struct tag_traits;
	template<typename T> struct system_traits;
	template<typename T> struct iter_traits;
}

namespace ecs {
	using index_t = ECS_HANDLE_INTEGRAL;
	using version_t = ECS_HANDLE_INTEGRAL;
	static constexpr std::size_t index_width = static_cast<std::size_t>(sizeof(index_t) * 8 * ECS_HANDLE_FACTOR);
	static constexpr std::size_t version_width = static_cast<std::size_t>(sizeof(index_t) * 8 * (1 - ECS_HANDLE_FACTOR));

	struct entity {
		constexpr entity(index_t ind = static_cast<index_t>(-1), version_t vers = 0) : index(ind), version(vers) { }
		constexpr bool operator==(const entity& other) const { return index == other.index && version == other.version; }

		index_t index : index_width;
		version_t version : version_width;
	};

	struct indirect {
		constexpr indirect(index_t ind = static_cast<index_t>(-1), version_t vers = 0) : index(ind), version(vers) { }
		
		index_t index : index_width;
		version_t version : version_width;
	};
}