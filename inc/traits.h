#pragma once
#include "fwd.h"
#include <tuple>

// traits
namespace ecs {
	template<typename T> struct res_traits { 
		using type = T;
		static constexpr id key = std::type_identity<std::remove_const_t<T>>{};
	};

	template<typename T, id key_V> struct res_traits<res<T, key_V>> { 
		using type = T;
		static constexpr id key = key_V;
	};
}

namespace ecs {
	template<typename T> struct from { static constexpr bool value = false; using type = T; };
	template<typename T> struct from<from<T>> { static constexpr bool value = true; using type = T; };
	template<typename T> struct from<const from<T>> { static constexpr bool value = true; using type = T; };

	template<typename T>
	struct tag_traits<const T> : tag_traits<T> { };

	template<typename T>
	struct tag_traits {
		static constexpr bool primary_mixin = true; // the tag is used to define the primary tag
		static constexpr bool updated_mixin = true; // the iter mixin is incremented and decremented
		static constexpr bool ordered_mixin = true; // the iter mixin can be used for iter comparisons
		static constexpr bool guarded_mixin = true; // the iter mixin guards the increment and decrement, converts the iter to a bidirectional iter
		static constexpr bool compose_mixin = true; // the iter mixin composes a ref and val mixin
		static constexpr bool exposed_mixin = true; // the ref and val mixin are exposed in the structured binding
		static constexpr bool compare_mixin = true; // the ref and val mixin can be used in ref/val comparisons. only a single mixin is compared
	};

	template<typename T>
	struct tag_traits<T*> {
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = true;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = false;
		static constexpr bool compose_mixin = true;
		static constexpr bool exposed_mixin = true;
		static constexpr bool compare_mixin = false;
	};
	template<> struct tag_traits<entity> {
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = true;
		static constexpr bool ordered_mixin = true;
		static constexpr bool guarded_mixin = false;
		static constexpr bool compose_mixin = true;
		static constexpr bool exposed_mixin = true;
		static constexpr bool compare_mixin = true;
	};
	template<> struct tag_traits<indirect> {
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = true;
		static constexpr bool ordered_mixin = true;
		static constexpr bool guarded_mixin = false;
		static constexpr bool compose_mixin = true;
		static constexpr bool exposed_mixin = false;
		static constexpr bool compare_mixin = false;
	};	
	template<typename T>
	struct tag_traits<from<T>> {
		static constexpr bool primary_mixin = true;
		static constexpr bool updated_mixin = false;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = false;
		static constexpr bool compose_mixin = false;
		static constexpr bool exposed_mixin = false;
		static constexpr bool compare_mixin = false;
	};
	template<typename ... Ps>
	struct tag_traits<inc<Ps...>>   { 
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = false;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = true;
		static constexpr bool compose_mixin = false;
		static constexpr bool exposed_mixin = false;
		static constexpr bool compare_mixin = false;
	};
	template<typename ... Ps>
	struct tag_traits<exc<Ps...>>   { 
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = false;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = true;
		static constexpr bool compose_mixin = false;
		static constexpr bool exposed_mixin = false;
		static constexpr bool compare_mixin = false;
	};
	template<typename ... Ps>
	struct tag_traits<any<Ps...>>   { 
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = false;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = true;
		static constexpr bool compose_mixin = false;
		static constexpr bool exposed_mixin = false;
		static constexpr bool compare_mixin = false;
	};
	template<id Flag>
	struct tag_traits<flag<Flag>>   { 
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = true;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = true;
		static constexpr bool compose_mixin = false;
		static constexpr bool exposed_mixin = false;
		static constexpr bool compare_mixin = false;
	};
	template<auto State>
	struct tag_traits<state<State>> { 
		static constexpr bool primary_mixin = false;
		static constexpr bool updated_mixin = true;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = true;
		static constexpr bool compose_mixin = false;
		static constexpr bool exposed_mixin = false;
		static constexpr bool compare_mixin = false;
	};
}

namespace ecs {
	template<typename T>
	struct system_traits : system_traits<decltype(&T::operator())> { };
	
	template<typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(arg_Ts...)> : system_traits<view<arg_Ts...>> { };

	template<typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(&)(arg_Ts...)> : system_traits<view<arg_Ts...>> { };

	template<typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(*)(arg_Ts...)> : system_traits<view<arg_Ts...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...)> : system_traits<view<arg_Ts...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...) const> : system_traits<view<arg_Ts...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...)&> : system_traits<view<arg_Ts...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...) const &> : system_traits<view<arg_Ts...>> { };

	template<typename T, typename ret_T, typename ... arg_Ts>
	struct system_traits<ret_T(T::*)(arg_Ts...) &&> : system_traits<view<arg_Ts...>> { };
	
	template<typename ... Ts>
	struct system_traits<view<Ts...>> { 
		using view_type = view<Ts...>;
		using iter_type = iter<std::remove_reference_t<Ts>...>;
	};
}

namespace ecs {
	template<template<typename...> typename Iter, typename ... Ts>
	struct iter_traits<Iter<Ts...>> {
		template<std::size_t I>
		static constexpr std::size_t exposed_mixin_index = []->std::size_t {
			// find nth compose_mixin where tag_traits<Ts>::exposed_mixin true
			if (std::size_t i = -1, j = -1; (((++i, tag_traits<Ts>::exposed_mixin && ++j == I)) || ...)) return i;
			return -1;
		}();

		static constexpr std::size_t compare_mixin_index = []->std::size_t {
			// find 1st type where tag_traits<Ts>::compare_mixin true and prefer entity
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::compare_mixin && !std::is_same_v<std::remove_const_t<Ts>, entity>) || ...)) return i;
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::compare_mixin) || ...)) return i;
			return -1;
		}();
		
		static constexpr std::size_t primary_mixin_index = []->std::size_t {
			// find 1st type where tag_traits<Ts>::primary_mixin true and prefer from<>
			if (std::size_t i = -1; ((++i, from<Ts>::value) || ...)) return i;
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::primary_mixin) || ...)) return i;
			return -1;
		}();
		
		static constexpr std::size_t ordered_mixin_index = []->std::size_t {
			// find first type where tag_traits<Ts>::ordered_mixin true and prefer indirect
			if (std::size_t i = -1; ((++i, std::is_same_v<std::remove_const_t<Ts>, indirect>) || ...)) return i;
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::ordered_mixin) || ...)) return i;
			return -1;
		}();
		
		template<std::size_t I>
		static constexpr std::size_t guarded_mixin_index = []->std::size_t {
			// find nth type where tag_traits<Ts>::exposed_mixin true
			if (std::size_t i = -1, j = -1; ((++i, tag_traits<Ts>::guarded_mixin && primary_mixin_index != i && ++j == I) || ...)) return i;
			return -1;
		}();
		
		template<std::size_t I>
		static constexpr std::size_t exposed_compose_index = []->std::size_t {
			// find nth compose_mixin where tag_traits<Ts>::exposed_mixin true
			if (std::size_t i = -1, j = -1; ((tag_traits<Ts>::compose_mixin && (++i, tag_traits<Ts>::exposed_mixin && ++j == I)) || ...)) return i;
			return -1;
		}();
		
		static constexpr std::size_t compare_compose_index = []->std::size_t {
			// find 1st type where tag_traits<Ts>::compare_mixin true and prefer entity
			if (std::size_t i = -1; ((tag_traits<Ts>::compose_mixin && (++i, tag_traits<Ts>::compare_mixin && !std::is_same_v<std::remove_const_t<Ts>, entity>)) || ...)) return i;
			if (std::size_t i = -1; ((tag_traits<Ts>::compose_mixin && (++i, tag_traits<Ts>::compare_mixin)) || ...)) return i;
			return -1;
		}();

		using primary_mixin = std::tuple_element_t<primary_mixin_index, std::tuple<Ts...>>;
		using ordered_mixin = std::tuple_element_t<ordered_mixin_index, std::tuple<Ts...>>;
		static constexpr bool guarded = ((tag_traits<Ts>::guarded_mixin && !std::is_same_v<Ts, primary_mixin>) || ...);
		using sentinel_mixin = iter_sentinel<std::remove_const_t<std::conditional_t<guarded, ordered_mixin, void>>>;

		using from_type = from<std::remove_const_t<primary_mixin>>::type;
		using primary_pool = std::conditional_t<std::is_const_v<primary_mixin>, const pool<from_type>, pool<from_type>>;

		using update_sequence = decltype([]<std::size_t ... Is>(std::type_identity<std::tuple<std::integral_constant<std::size_t, Is>...>>) {
			// filters update_mixin, reorders entity to front
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
				std::conditional_t<tag_traits<Ts>::updated_mixin, 
				std::tuple<std::integral_constant<std::size_t, Is>>,
				std::tuple<>>{}... 
			))>{};
		}(std::make_index_sequence<sizeof...(Ts)>{})));

		using guard_sequence = decltype([]<std::size_t ... Is>(std::index_sequence<Is...>) {
			return []<typename... Js>(std::type_identity<std::tuple<Js...>>) {
				return std::index_sequence<Js::value...>{};
			}(std::type_identity<decltype(std::tuple_cat(
				std::conditional_t<tag_traits<Ts>::guarded_mixin && !std::is_same_v<primary_mixin, Ts>, 
					std::tuple<std::integral_constant<std::size_t, Is>>, 
					std::tuple<>>{}...
			))>{});
		}(std::make_index_sequence<sizeof...(Ts)>{}));

		using exposed_sequence = std::index_sequence<[]<typename T>(std::type_identity<T>)->std::size_t {
			if (std::size_t i = -1; (((tag_traits<Ts>::exposed_mixin && ++i), std::is_same_v<T, Ts>) || ...)) return i;
			return -1;
		}(std::type_identity<Ts>{})...>;

		using compose_sequence = decltype([]<std::size_t ... Is>(std::index_sequence<Is...>) {
			return []<typename... Js>(std::type_identity<std::tuple<Js...>>) {
				return std::index_sequence<Js::value...>{};
			}(std::type_identity<decltype(std::tuple_cat(
				std::conditional_t<tag_traits<Ts>::compose_mixin, 
					std::tuple<std::integral_constant<std::size_t, Is>>, 
					std::tuple<>>{}...
			))>{});
		}(std::make_index_sequence<sizeof...(Ts)>{}));
	};	
}