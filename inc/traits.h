#pragma once
#include "fwd.h"

// traits
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
	struct system_traits<view<Ts...>> { using type = view<std::remove_reference_t<Ts>...>; };
	
	template<typename T>
	struct iter_traits;
	

	template<template<typename...> typename Iter, typename ... Ts>
	struct iter_traits<Iter<Ts...>> {
		template<std::size_t I>
		static constexpr std::size_t binding_mixin_index = []->std::size_t { // find nth type where tag_traits<Ts>::binding_mixin true
			if (std::size_t i = -1, j = -1; ((++i, tag_traits<Ts>::binding_mixin && ++j == I) || ...)) return i;
			return -1;
		}();
		static constexpr std::size_t primary_mixin_index = []->std::size_t { // find 1st type where tag_traits<Ts>::primary_mixin true and prefer from<>
			if (std::size_t i = -1; ((++i, from<Ts>::value) || ...)) return i;
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::primary_mixin) || ...)) return i;
			return -1;
		}();
		static constexpr std::size_t compare_mixin_index = []->std::size_t { // find 1st type where tag_traits<Ts>::compare_mixin true and prefer entity
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::compare_mixin && !std::is_same_v<std::remove_const_t<Ts>, entity>) || ...)) return i;
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::compare_mixin) || ...)) return i;
			return -1;
		}();
		static constexpr std::size_t ordered_mixin_index = []->std::size_t { // find first type where tag_traits<Ts>::ordered_mixin true and prefer indirect
			if (std::size_t i = -1; ((++i, std::is_same_v<std::remove_const_t<Ts>, indirect>) || ...)) return i;
			if (std::size_t i = -1; ((++i, tag_traits<Ts>::ordered_mixin) || ...)) return i;
			return -1;
		}();


		template<std::size_t I>
		using binding_mixin = std::tuple_element_t<binding_mixin_index<I>, std::tuple<Ts...>>;
		using primary_mixin = std::tuple_element_t<primary_mixin_index, std::tuple<Ts...>>;
		using compare_mixin = std::tuple_element_t<compare_mixin_index, std::tuple<Ts...>>;
		using ordered_mixin = std::tuple_element_t<ordered_mixin_index, std::tuple<Ts...>>;
		
		template<std::size_t I>
		static constexpr std::size_t guarded_mixin_index = []->std::size_t { // find nth type where tag_traits<Ts>::binding_mixin true
			if (std::size_t i = -1, j = -1; ((++i, tag_traits<Ts>::guarded_mixin && !std::is_same_v<Ts, primary_mixin> && ++j == I) || ...)) return i;
			return -1;
		}();

		static constexpr std::size_t guarded = ((tag_traits<Ts>::guarded_mixin && !std::is_same_v<Ts, primary_mixin>) + ...);

		using guard_sequence = decltype([]<std::size_t ... Is>(std::index_sequence<Is...>) { 
			return std::index_sequence<guarded_mixin_index<Is>...>{};
		}(std::make_index_sequence<guarded>{}));

		using update_sequence = decltype([]{
			static constexpr std::size_t entity_mixin_index = []->std::size_t { // find entity index
				if (std::size_t i = -1; ((++i, std::is_same_v<std::remove_const_t<Ts>, entity>) || ...)) return i;
				return -1;
			}();
			if constexpr (entity_mixin_index == -1) { // if no entity exists, return 0... N
				return std::make_index_sequence<sizeof...(Ts)>{};
			} else { // replace first element with entity, push back elements before entity index, push back elements after entity index
				return []<std::size_t ... Is, std::size_t ... Js>(std::index_sequence<Is...>, std::index_sequence<Js...>) {
					return std::index_sequence<entity_mixin_index, Is..., (Js + entity_mixin_index + 1)...>{};
				}(std::make_index_sequence<entity_mixin_index>{}, std::make_index_sequence<sizeof...(Ts) - entity_mixin_index - 1>{});
			}
		}());

		using sentinel_mixin = iter_sentinel<std::remove_const_t<std::conditional_t<guarded, ordered_mixin, void>>>;

		using primary_pool = std::conditional_t<std::is_const_v<primary_mixin>, const ecs::pool<std::remove_const_t<primary_mixin>>, ecs::pool<primary_mixin>>;
	};

	template<typename T>
	struct tag_traits<const T> : tag_traits<T> { };

	template<typename T>
	struct tag_traits {
		static constexpr bool primary_mixin = true;
		static constexpr bool binding_mixin = true;
		static constexpr bool compare_mixin = true;
		static constexpr bool ordered_mixin = true;
		static constexpr bool guarded_mixin = true;
	};

	template<typename T>
	struct tag_traits<T*> {
		static constexpr bool primary_mixin = false;
		static constexpr bool binding_mixin = true;
		static constexpr bool compare_mixin = false;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = false;
	};

	template<> struct tag_traits<entity> {
		static constexpr bool primary_mixin = false;
		static constexpr bool binding_mixin = true;
		static constexpr bool compare_mixin = true;
		static constexpr bool ordered_mixin = true;
		static constexpr bool guarded_mixin = false;
	};

	template<> struct tag_traits<indirect> {
		static constexpr bool primary_mixin = false;
		static constexpr bool binding_mixin = false;
		static constexpr bool compare_mixin = false;
		static constexpr bool ordered_mixin = true;
		static constexpr bool guarded_mixin = false;
	};
	
	template<typename T>
	struct tag_traits<from<T>> {
		static constexpr bool primary_mixin = true;
		static constexpr bool binding_mixin = false;
		static constexpr bool compare_mixin = false;
		static constexpr bool ordered_mixin = true;
		static constexpr bool guarded_mixin = true;
	};

	template<auto P> struct tag_traits<pred<P>> {
		static constexpr bool primary_mixin = false;
		static constexpr bool binding_mixin = false;
		static constexpr bool compare_mixin = false;
		static constexpr bool ordered_mixin = false;
		static constexpr bool guarded_mixin = true;
	};

	// tag traits correspond to predicate iterator mixins
	template<typename ... Ps> struct tag_traits<inc<Ps...>> : tag_traits<pred<nullptr>> { };
	template<typename ... Ps> struct tag_traits<exc<Ps...>> : tag_traits<pred<nullptr>> { };
	template<typename ... Ps> struct tag_traits<any<Ps...>> : tag_traits<pred<nullptr>> { };
	template<typename ... Ps> struct tag_traits<none<Ps...>> : tag_traits<pred<nullptr>> { };
	template<id F> struct tag_traits<flag<F>> : tag_traits<pred<nullptr>> { };
	template<auto S> struct tag_traits<state<S>> : tag_traits<pred<nullptr>> { };
}