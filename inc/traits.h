#pragma once
#include "fwd.h"
#include <tuple>

// traits
namespace ecs {
	template<typename T>
	struct res_traits { 
		using type = T;
		static constexpr label key = std::type_identity<std::remove_const_t<T>>{};
	};

	template<typename T, label key_V>
	struct res_traits<res<T, key_V>> { 
		using type = T;
		static constexpr label key = key_V;
	};
}

namespace ecs {
	template<typename T> struct from { static constexpr bool value = false; using type = T; };
	template<typename T> struct from<from<T>> { static constexpr bool value = true; using type = T; };
	template<typename T> struct from<const from<T>> { static constexpr bool value = true; using type = T; };

	// used for tag selection, not for iter characterisation
	template<typename T> struct tag_traits<const T>
	 : tag_traits<T> { };

	template<typename T> struct tag_traits {
		static constexpr uint32_t flags = tag::primary | tag::updated | tag::ordered | tag::guarded | tag::compose | tag::exposed | tag::compare;
	};
	template<> struct tag_traits<entity> {
		static constexpr uint32_t flags =                tag::updated | tag::ordered                | tag::compose | tag::exposed | tag::compare;
	};
	template<> struct tag_traits<indirect> {
		static constexpr uint32_t flags =                tag::updated | tag::ordered                | tag::compose;
	};	
	template<typename T> struct tag_traits<T*> {
		static constexpr uint32_t flags =                tag::updated                               | tag::compose | tag::exposed;
	};
	template<typename T, label Name> struct tag_traits<res<T, Name>> {
		static constexpr uint32_t flags =                                                             tag::compose | tag::exposed;
	};
	template<typename T> struct tag_traits<from<T>> {
		static constexpr uint32_t flags = 0;
	};
	template<typename ... Ps> struct tag_traits<inc<Ps...>>   { 
		static constexpr uint32_t flags =                                              tag::guarded;
	};
	template<typename ... Ps> struct tag_traits<exc<Ps...>> { 
		static constexpr uint32_t flags =                                              tag::guarded;
	};
	template<typename ... Ps> struct tag_traits<any<Ps...>> { 
		static constexpr uint32_t flags =                                              tag::guarded;
	};
	template<label Flag> struct tag_traits<flag<Flag>> {
		static constexpr uint32_t flags =                tag::updated                | tag::guarded;
	};
	template<auto State> struct tag_traits<state<State>> { 
		static constexpr uint32_t flags =                tag::updated                | tag::guarded;
	};
}

namespace ecs {
	template<typename T>
	struct system_traits : system_traits<decltype(&T::operator())> { };
	
	template<typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(arg_Ts...)> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };

	template<typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(&)(arg_Ts...)> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };

	template<typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(*)(arg_Ts...)> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...)> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...) const> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...)&> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...) const &> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...) &&> : system_traits<view<std::remove_reference_t<arg_Ts>...>> { };
	
	template<typename ... Ts>
	struct system_traits<view<Ts...>> { 
		using view_type = view<Ts...>;
		using iter_type = iter<Ts...>;
	};
}

namespace ecs {
	template<template<typename...> typename Iter, typename ... Ts>
	struct iter_traits<Iter<Ts...>> {

		// mixin unitary op selection

		// mixin can be used as default from type
		static constexpr std::size_t primary_mixin_index = []->std::size_t {
			// find 1st type where tag_traits<Ts>::primary_mixin true and prefer from<>
			if (std::size_t i = -1; ((++i, from<Ts>::value) || ...)) return i;
			if (std::size_t i = -1; ((++i, ((tag_traits<Ts>::flags & tag::primary) == tag::primary)) || ...)) return i;
			return -1;
		}();

		// mixin can be used for iter compare operations
		static constexpr std::size_t ordered_mixin_index = []->std::size_t {
			// find first type where tag_traits<Ts>::ordered_mixin true and prefer indirect
			if (std::size_t i = -1; ((++i, std::is_same_v<std::remove_const_t<Ts>, indirect>) || ...)) return i;
			if (std::size_t i = -1; ((++i, ((tag_traits<Ts>::flags & tag::ordered) == tag::ordered)) || ...)) return i;
			return -1;
		}();

		// mixin is used for compare operation
		static constexpr std::size_t compare_mixin_index = []->std::size_t {
			// find 1st type where tag_traits<Ts>::compare_mixin true and prefer not entity
			if (std::size_t i = -1; ((++i, ((tag_traits<Ts>::flags & tag::compare) == tag::compare) && !std::is_same_v<std::remove_const_t<Ts>, entity>) || ...)) return i;
			if (std::size_t i = -1; ((++i, ((tag_traits<Ts>::flags & tag::compare) == tag::compare)) || ...)) return i;
			return -1;
		}();
		
		
		// mixin set op filtering

		// mixin is exposed in structured binding
		template<std::size_t I>
		static constexpr std::size_t exposed_mixin_index = []->std::size_t {
			// find nth compose_mixin where tag exposed
			if (std::size_t i = -1, j = -1; (((++i, ((tag_traits<Ts>::flags & tag::exposed) == tag::exposed) && ++j == I)) || ...)) return i;
			return -1;
		}();

		template<std::size_t I>
		static constexpr std::size_t guarded_mixin_index = []->std::size_t {
			// find nth type where tag guarded
			if (std::size_t i = -1, j = -1; ((++i, ((tag_traits<Ts>::flags & tag::guarded) == tag::guarded) && primary_mixin_index != i && ++j == I) || ...)) return i;
			return -1;
		}();
		
		template<std::size_t I>
		static constexpr std::size_t exposed_compose_index = []->std::size_t {
			// find nth compose_mixin where tag_traits<Ts>::exposed_mixin true
			if (std::size_t i = -1, j = -1; (( ((tag_traits<Ts>::flags & tag::compose) == tag::compose) && (++i, ((tag_traits<Ts>::flags & tag::exposed) | tag::exposed) && ++j == I)) || ...)) return i;
			return -1;
		}();
		
		static constexpr std::size_t compare_compose_index = []->std::size_t {
			// from composed mixins find 1st tag where  true and prefer entity
			if (std::size_t i = -1; (( ((tag_traits<Ts>::flags & tag::compose) == tag::compose) && (++i, ((tag_traits<Ts>::flags & tag::compare) == tag::compare) && 
				!std::is_same_v<std::remove_const_t<Ts>, entity>)
			) || ...)) return i;
			if (std::size_t i = -1; (( ((tag_traits<Ts>::flags & tag::compose) == tag::compose) && (++i, ((tag_traits<Ts>::flags & tag::compare) == tag::compare))
			) || ...)) return i;
			return -1;
		}();
		
		using primary_mixin = std::tuple_element_t<primary_mixin_index, std::tuple<Ts...>>;
		using ordered_mixin = std::tuple_element_t<ordered_mixin_index, std::tuple<Ts...>>;
		
		static constexpr bool guarded = ((
			((tag_traits<Ts>::flags & tag::guarded) == tag::guarded) &&
			!std::is_same_v<Ts, primary_mixin>
		) || ...);
		
		using sentinel_mixin = iter_sentinel<std::remove_const_t<std::conditional_t<guarded, ordered_mixin, void>>>;
		
		using from_type = from<std::remove_const_t<primary_mixin>>::type;
		using primary_pool = std::conditional_t<std::is_const_v<primary_mixin>, const pool<from_type>, pool<from_type>>;

		using update_sequence = decltype([]<std::size_t ... Is>(std::type_identity<std::tuple<std::integral_constant<std::size_t, Is>...>>) {
			// filters for update_mixin, reorders entity to front
			static constexpr std::size_t entity_index = []->std::size_t { // find entity index
				if (std::size_t i = -1; ((++i, std::is_same_v<std::remove_const_t<std::tuple_element_t<Is, std::tuple<Ts...>>>, entity>) || ...)) return i;
				return -1;
			}();
			if constexpr (entity_index > sizeof...(Ts)) {
				return std::index_sequence<Is...>{};
			}
			else { // replace first element with entity, push elements before entity index, push elements after entity index
				return []<std::size_t ... Pre_Is, std::size_t ... Post_Is>(std::index_sequence<Pre_Is...>, std::index_sequence<Post_Is...>) {
					return std::index_sequence<
						(std::tuple_element_t<entity_index, std::tuple<std::integral_constant<std::size_t, Is>...>>::value), 
						(std::tuple_element_t<Pre_Is,       std::tuple<std::integral_constant<std::size_t, Is>...>>::value)..., 
						(std::tuple_element_t<Post_Is,      std::tuple<std::integral_constant<std::size_t, Is>...>>::value + entity_index + 1)...>{};
				}(std::make_index_sequence<entity_index>{}, std::make_index_sequence<sizeof...(Is) - entity_index - 1>{});
			}
		}([]<std::size_t ... Is>(std::index_sequence<Is...>) {
			return std::type_identity<decltype(std::tuple_cat(
				std::conditional_t<(((tag_traits<Ts>::flags & tag::updated) == tag::updated)), 
				std::tuple<std::integral_constant<std::size_t, Is>>,
				std::tuple<>>{}... 
			))>{};
		}(std::make_index_sequence<sizeof...(Ts)>{})));

		using guard_sequence = decltype([]<std::size_t ... Is>(std::index_sequence<Is...>) {
			return []<typename... Js>(std::type_identity<std::tuple<Js...>>) {
				return std::index_sequence<Js::value...>{};
			}(std::type_identity<decltype(std::tuple_cat(
				std::conditional_t<((tag_traits<Ts>::flags & tag::guarded) == tag::guarded) && !std::is_same_v<primary_mixin, Ts>, 
					std::tuple<std::integral_constant<std::size_t, Is>>, 
					std::tuple<>>{}...
			))>{});
		}(std::make_index_sequence<sizeof...(Ts)>{}));

		using exposed_sequence = std::index_sequence<[]<typename T>(std::type_identity<T>)->std::size_t {
			if (std::size_t i = -1; ((( ((tag_traits<Ts>::flags & tag::exposed) == tag::exposed) && ++i), std::is_same_v<T, Ts>) || ...)) return i;
			return -1;
		}(std::type_identity<Ts>{})...>;

		using compose_sequence = decltype([]<std::size_t ... Is>(std::index_sequence<Is...>) {
			return []<typename... Js>(std::type_identity<std::tuple<Js...>>) {
				return std::index_sequence<Js::value...>{};
			}(std::type_identity<decltype(std::tuple_cat(
				std::conditional_t<((tag_traits<Ts>::flags & tag::compose) == tag::compose), 
					std::tuple<std::integral_constant<std::size_t, Is>>, 
					std::tuple<>>{}...
			))>{});
		}(std::make_index_sequence<sizeof...(Ts)>{}));
	};
}