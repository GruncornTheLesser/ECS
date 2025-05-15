#pragma once
#include <string_view>
#include <array>
#include <algorithm>
#include <utility>
#include <type_traits>


#if defined(__clang__)
	#define PF_CMD __PRETTY_FUNCTION__
	#define PF_PREFIX "std::basic_string_view<char> ecs::meta::type_name() [T = "
	#define PF_SUFFIX "]"
#elif defined(__GNUC__) && !defined(__clang__)
	#define PF_CMD __PRETTY_FUNCTION__
	#define PF_PREFIX "constexpr std::basic_string_view<char> ecs::meta::type_name() [with T = "
	#define PF_SUFFIX "]"
#elif defined(_MSC_VER)
	#define PF_CMD __FUNCSIG__
	#define PF_PREFIX "struct std::basic_string_view<char> __cdecl ecs::meta::type_name<"
	#define PF_SUFFIX ">(void)"
#else
	#error "No support for this compiler."
#endif
namespace ecs::meta { // NOTE: when changing the namespace of this func you must update the macros for pretty function
	template<typename T> constexpr std::basic_string_view<char> type_name() {
		return { PF_CMD + sizeof(PF_PREFIX) - 1, sizeof(PF_CMD) + 1 - sizeof(PF_PREFIX) - sizeof(PF_SUFFIX) };
	}
	template<typename T> struct get_type_name { static constexpr std::basic_string_view<char> value = type_name<T>(); };
}
#undef PF_CMD
#undef PF_PREFIX
#undef PF_SUFFIX

namespace ecs::meta {
	/**
	 * @brief counts the number of variadic arguments in a type. Extends std::tuple_size to allow use on any variadic tuple like template.
	 */
	template<typename Tup> struct arg_count;
	template<typename Tup> static constexpr std::size_t arg_count_v = arg_count<Tup>::value;

	/**
	 * @brief selects a Nth element of variadic type. extends std::tuple_element to allow use on any variadic tuple like template.
	 */
	template<std::size_t N, typename Tup> struct arg_element;
	template<std::size_t N, typename Tup> using arg_element_t = typename arg_element<N, Tup>::type;
	
	/**
	 * @brief allows the passing of arguments to variadic templates.
	 */
	template<typename T, typename ... Ts> struct arg_append;
	template<typename ... Ts> struct arg_append_ { template<typename T> using type = arg_append<T, Ts...>; };
	template<typename T, typename ... Ts> using arg_append_t = typename arg_append<T, Ts...>::type;
	/**
	 * @brief 
	 */
	template<typename T, template<typename...> typename ... Eval_Tps> struct eval;
	template<template<typename...> typename ... Eval_Tps> struct eval_ { template<typename T> using type = eval<T, Eval_Tps...>; };
	template<typename T, template<typename...> typename ... Eval_Tps> using eval_t = typename eval<T, Eval_Tps...>::type;

	/**
	 * @brief conditionally evaluates T on predicate Pred_Tp with Eval_Tp. Extends std::conditional to lazily evaluate types allowing for guarded evaluation.
	 */
	template<typename T, template<typename...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp=std::type_identity> struct eval_if;
	template<template<typename...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp=std::type_identity> struct eval_if_{ template<typename T> using type = eval_if<T, Pred_Tp, If_Tp, Else_Tp>; };
	template<typename T, template<typename...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp=std::type_identity> using eval_if_t = typename eval_if<T, Pred_Tp, If_Tp, Else_Tp>::type;
	/**
	 * @brief 
	 */
	template<typename T, template<typename...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp, template<typename...> typename ... Eval_Tps> using eval_pre_if = eval_if<T, Pred_Tp, eval_<Eval_Tps..., If_Tp>::template type, eval_<Eval_Tps..., Else_Tp>::template type>;
	template<template<typename...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp, template<typename...> typename ... Eval_Tps> struct eval_pre_if_{ template<typename T> using type = eval_pre_if<T, Pred_Tp, If_Tp, Else_Tp, Eval_Tps...>; };
	template<typename T, template<typename...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp, template<typename...> typename ... Eval_Tps> using eval_pre_if_t = typename eval_pre_if<T, Pred_Tp, If_Tp, Else_Tp, Eval_Tps...>::type;


	struct eval_failure { };
	namespace details { template<typename T, template<typename...> typename eval_Tp, typename V, typename D, typename ... Arg_Ts> struct eval_try; };
	/**
	 * @brief tries to evaluate Eval_Tp<T, Arg_Ts...>::type, if evaluation fails type = D, D defaults to eval_failure
	 */
	template<typename T, template<typename...> typename Eval_Tp, typename D=eval_failure, typename ... Arg_Ts> using eval_try = details::eval_try<T, Eval_Tp, void, D, Arg_Ts...>;
	template<template<typename...> typename Eval_Tp, typename D=eval_failure> struct eval_try_ { template<typename T, typename ... Arg_Ts> using type = eval_try<T, Eval_Tp, D, Arg_Ts...>; };
	template<typename T, template<typename...> typename Eval_Tp, typename D=eval_failure, typename ... Arg_Ts> using eval_try_t = typename eval_try<T, Eval_Tp, D, Arg_Ts...>::type;


	/**
	 * @brief template meta program to apply transform template struct to eval_each type in tuple
	 * @tparam Tup tuple set of types to transform
	 * @tparam Eval_Tp evaluate template struct
	 * @tparam Ts arguments to pass to transform template struct
	 */
	 template<typename Tup, template<typename...> typename ... Eval_Tp> struct eval_each;
	 template<template<typename...> typename ... Eval_Tp> struct eval_each_ { template<typename Tup> using type = eval_each<Tup, Eval_Tp...>; };
	 template<typename Tup, template<typename...> typename ... Eval_Tp> using eval_each_t = typename eval_each<Tup, Eval_Tp...>::type;

	/**
	 * @brief template meta program to conjuct tuple elements with set
	 */
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> struct conjunction;
	template<template<typename...> typename Pred_Tp> struct conjunction_ { template<typename Tup, typename ... Arg_Ts> using type = conjunction<Tup, Pred_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> static constexpr bool conjunction_v = conjunction<Tup, Pred_Tp, Arg_Ts...>::value;
	
	/**
	 * @brief template meta program to disjunct tuple elements 
	 */
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Ts> struct disjunction;
	template<template<typename...> typename Pred_Tp> struct disjunction_ { template<typename Tup, typename ... Arg_Ts> using type = disjunction<Tup, Pred_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Ts> static constexpr bool disjunction_v = disjunction<Tup, Pred_Tp, Ts...>::value;

	/**
	 * @brief concatenated tuples together, the first tuple template type is used as the tuple container
	 */
	template<typename ... Tups> struct concat;
	template<typename Tup> struct concat_ { template<typename ... Tups> using type = concat<Tup, Tups...>; };
	template<typename ... Tups> using concat_t = typename concat<Tups...>::type;

	namespace details { template<typename Tup, typename Ind=std::make_index_sequence<arg_count_v<Tup>>> struct reverse; };

	/**
	 * @brief reverses the type order of elements in a tuple 
	 */
	template<typename Tup> using reverse = details::reverse<Tup>;
	template<typename Tup> using reverse_t = typename reverse<Tup>::type;


	namespace details { template<typename Out, typename Tup, template<typename...> typename Same_Tp, typename ... Arg_Ts> struct unique; };
	/**
	 * @brief removes duplicate types in Tup
	 */
	template<typename Tup, template<typename...> typename Same_Tp=std::is_same, typename ... Arg_Ts> using unique = details::unique<std::tuple<>, Tup, Same_Tp, Arg_Ts...>;
	template<template<typename...> typename Same_Tp=std::is_same, typename ... Arg_Ts> struct unique_ { template<typename Tup> using type = unique<Tup, Same_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Same_Tp=std::is_same, typename ... Arg_Ts> using unique_t = typename unique<Tup, Same_Tp, Arg_Ts...>::type;

	namespace details { template<typename Out, typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> struct filter; };
	/**
	 * @brief removes types that do not match the predicate template struct 
	 */
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> using filter = details::filter<std::tuple<>, Tup, Pred_Tp, Arg_Ts...>;
	template<template<typename...> typename Pred_Tp, typename ... Arg_Ts> struct filter_ { template<typename Tup> using type = filter<Tup, Pred_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> using filter_t = typename filter<Tup, Pred_Tp, Arg_Ts...>::type;
	
	namespace details { template<typename Out, typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> struct remove_if; };
	/**
	 * @brief removes types that match the predicate template struct 
	 */
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> using remove_if = details::remove_if<std::tuple<>, Tup, Pred_Tp, Arg_Ts...>;
	template<template<typename...> typename Pred_Tp, typename ... Arg_Ts> struct remove_if_ { template<typename Tup> using type = remove_if<Tup, Pred_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> using remove_if_t = typename remove_if<Tup, Pred_Tp, Arg_Ts...>::type;

	/**
	 * @brief finds first type/index that match the predicate template struct 
	 */
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> struct find_if;
	template<template<typename...> typename Pred_Tp, typename ... Arg_Ts> struct find_if_ { template<typename Tup> using type = find_if<Tup, Pred_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> using find_if_t = typename find_if<Tup, Pred_Tp>::type;
	template<typename Tup, template<typename...> typename Pred_Tp, typename ... Arg_Ts> static constexpr bool find_if_v = find_if<Tup, Pred_Tp>::value;

	namespace details { template<typename Tup, typename Ind, template<typename...> typename ... Eval_Tps> struct sort_by; };

	/**
	 * @brief sorts a tuple by a value getter template struct
	 */
	template<typename Tup, template<typename> typename ... Eval_Tps> using sort_by = details::sort_by<Tup, std::make_index_sequence<arg_count_v<Tup>>, Eval_Tps...>;
	template<template<typename> typename ... Eval_Tps> struct sort_by_ { template<typename Tup> using type = sort_by<Tup, Eval_Tps...>; };
	template<typename Tup, template<typename> typename ... Eval_Tps> using sort_by_t = typename sort_by<Tup, Eval_Tps...>::type;

	namespace details { template<typename Tup, template<typename...> typename Eval_Tp, typename Ind, typename ... Arg_Ts> struct min_by; };
	
	/**
	 * @brief finds a type by the minimum value evaluated from the getter template struct
	 */
	template<typename Tup, template<typename...> typename Eval_Tp=get_type_name, typename ... Arg_Ts> using min_by = details::min_by<Tup, Eval_Tp, std::make_index_sequence<arg_count_v<Tup>>, Arg_Ts...>;
	template<template<typename...> typename Eval_Tp=get_type_name, typename ... Arg_Ts> struct min_by_ { template<typename Tup> using type = min_by<Tup, Eval_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Eval_Tp=get_type_name, typename ... Arg_Ts> using min_by_t = typename min_by<Tup, Eval_Tp, Arg_Ts...>::type;

	namespace details { template<typename Tup, template<typename...> typename Eval_Tp, typename Ind, typename ... Arg_Ts> struct max_by; };

	/**
	 * @brief finfs a type by the maximum value evaluated from the getter template struct 
	 */
	template<typename Tup, template<typename...> typename Eval_Tp=get_type_name, typename ... Arg_Ts> using max_by = details::max_by<Tup, Eval_Tp, std::make_index_sequence<arg_count_v<Tup>>, Arg_Ts...>;
	template<template<typename...> typename Eval_Tp=get_type_name, typename ... Arg_Ts> struct max_by_ { template<typename Tup> using type = max_by<Tup, Eval_Tp, Arg_Ts...>; };
	template<typename Tup, template<typename...> typename Eval_Tp=get_type_name, typename ... Arg_Ts> using max_by_t = typename max_by<Tup, Eval_Tp, Arg_Ts...>::type;


	namespace details { 
		template<typename Tup, typename Func_T, typename ... Arg_Ts, std::size_t ... Is> 
		inline constexpr decltype(auto) apply(Func_T&& func, std::index_sequence<Is...>, Arg_Ts&& ... args); 
	}

	/**
	 * @brief invoke callable object func with the template arguments from Tup, differs from std::apply, by not requiring tuple value
	 */
	template<typename Tup, typename Func_T, typename ... Arg_Ts> 
	inline constexpr decltype(auto) apply(Func_T&& func, Arg_Ts&& ... args);

	template<typename Tup, typename Ret_T, typename Func_T, typename ... Arg_Ts>
	inline constexpr decltype(auto) apply_each(Func_T&& func, Arg_Ts&& ... args) {
		return apply<Tup>([&]<typename ... Ts>{ return Ret_T{ func.template operator()<Ts>()... }; });
	}

	/**
	 * @brief TODO: THIS DOESNT MATCH THE SYNTAX/FORMAT OF THE OTHER META FUNCS
	 */
	template<typename T, template<typename...> typename Tp> struct wrap { using type = Tp<T>; };
	template<template<typename...> typename Tp> struct wrap_ { template<typename T> using type = wrap<T, Tp>; };
	template<typename T, template<typename...> typename Tp> using wrap_t = wrap<T, Tp>::type;
	
	/**
	 * @brief 
	 * 
	 * @tparam T 
	 */
	template<typename T> using unwrap = arg_element<0, T>;
	template<typename T> using unwrap_t = unwrap<T>::type;
	
	/**
	* @brief TODO: THIS DOESNT MATCH THE SYNTAX/FORMAT OF THE OTHER META FUNCS
	* 
	* @tparam T 
	* @tparam Tp 
	*/
	template<typename T, template<typename...> typename Tp> struct rewrap;
	template<template<typename...> typename _Tp, typename ... Ts, template<typename...> typename Tp> struct rewrap<_Tp<Ts...>, Tp> { using type = Tp<Ts...>; };
	template<template<typename...> typename Tp> struct rewrap_ { template<typename T> using type = rewrap<T, Tp>; };
	template<typename T, template<typename...> typename Tp> using rewrap_t = rewrap<T, Tp>::type;


	template<template<typename ...> typename Pred_Tp, typename ... Arg_Ts> struct negate { template<typename T> using type = Pred_Tp<T, Arg_Ts...>; };
	template<template<typename ...> typename ... Pred_Tp> struct any_of { template<typename ... Ts> using type = std::disjunction<Pred_Tp<Ts...>...>; };
	template<template<typename ...> typename ... Pred_Tp> struct all_of { template<typename ... Ts> using type = std::conjunction<Pred_Tp<Ts...>...>; };

	template<template<typename ...> typename Pred_Tp, template<typename ...> typename ... Eval_Tps> struct pred { 
		template<typename T, typename ... Arg_Ts> using type = Pred_Tp<eval_t<T, Eval_Tps...>, Arg_Ts...>;
	};

	namespace cmp {
		template<typename T1, typename T2> struct lt { static constexpr bool value = T1::value < T2::value; };
		template<typename T1, typename T2> struct lteq { static constexpr bool value = T1::value <= T2::value; };
		template<typename T1, typename T2> struct gt { static constexpr bool value = T1::value > T2::value; };
		template<typename T1, typename T2> struct gteq { static constexpr bool value = T1::value >= T2::value; };
		template<typename T1, typename T2> struct eq { static constexpr bool value = T1::value == T2::value; };
		template<typename T1, typename T2> struct neq { static constexpr bool value = T1::value != T2::value; };
	}
}

namespace ecs::meta {
	template<template<typename...> typename Tp, typename ... Ts> 
	struct arg_count<Tp<Ts...>> { static constexpr std::size_t value = sizeof...(Ts); };

	template<std::size_t N, template<typename...> typename Tp, typename T, typename ... Ts>
	struct arg_element<N, Tp<T, Ts...>> : arg_element<N - 1u, Tp<Ts...>> { };

	template<template<typename...> typename Tp, typename T, typename ... Ts>
	struct arg_element<0, Tp<T, Ts...>> { using type = T; };

	template<template<typename ...> typename Tp, typename ... Ts, typename ... Arg_Ts> struct arg_append<Tp<Ts...>, Arg_Ts...> { 
		using type = Tp<Arg_Ts..., Ts...>; 
	};

	template<typename T, template<typename...> typename Eval_Tp, template<typename...> typename ... Eval_Tps>
	struct eval<T, Eval_Tp, Eval_Tps...> { using type = eval_t<typename Eval_Tp<T>::type, Eval_Tps...>; };

	template<typename T, template<typename...> typename Eval_Tp>
	struct eval<T, Eval_Tp> { using type = typename Eval_Tp<T>::type; };

	template<typename T>
	struct eval<T> { using type = T; };

	template<typename T, template<typename ...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp> requires (Pred_Tp<T>::value)
	struct eval_if<T, Pred_Tp, If_Tp, Else_Tp> : If_Tp<T> { };

	template<typename T, template<typename ...> typename Pred_Tp, template<typename...> typename If_Tp, template<typename...> typename Else_Tp> requires (!Pred_Tp<T>::value)
	struct eval_if<T, Pred_Tp, If_Tp, Else_Tp> : Else_Tp<T> { };
	
	namespace details { 
		template<typename T, template<typename...> typename Eval_Tp, typename V, typename D, typename ... Arg_Ts> 
		struct eval_try { using type = D; };

		template<typename T, template<typename...> typename Eval_Tp, typename D, typename ... Arg_Ts> 
		struct eval_try<T, Eval_Tp, std::void_t<typename Eval_Tp<T, Arg_Ts...>::type>, D, Arg_Ts...> : Eval_Tp<T, Arg_Ts...> { };
	};

	template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename ... Eval_Tps> 
	struct eval_each<Tp<Ts...>, Eval_Tps...> { using type = Tp<eval_t<Ts, Eval_Tps...>...>; };

	template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Pred_Tp, typename ... Arg_Ts> 
	struct conjunction<Tp<Ts...>, Pred_Tp, Arg_Ts...> : std::conjunction<Pred_Tp<Ts, Arg_Ts...>...> { };

	template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Pred_Tp, typename ... Arg_Ts> 
	struct disjunction<Tp<Ts...>, Pred_Tp, Arg_Ts...> : std::disjunction<Pred_Tp<Ts, Arg_Ts...>...> { };

	template<template<typename ...> typename Tp, typename ... Ts, typename ... Us, typename ... Tups>
	struct concat<Tp<Ts...>, Tp<Us...>, Tups...> { using type = typename concat<Tp<Ts..., Us...>, Tups...>::type; };

	template<template<typename ...> typename Tp, typename ... Ts>
	struct concat<Tp<Ts...>> { using type = Tp<Ts...>; };

	template<>
	struct concat<> { using type = std::tuple<>; };

	namespace details {
		template<template<typename ...> typename Tp, typename ... Ts, size_t ... Is>
		struct reverse<Tp<Ts...>, std::index_sequence<Is...>> { 
			using type = Tp<arg_element_t<sizeof...(Ts) - Is - 1, Tp<Ts...>>...>; 
		};
	}


	namespace details {
		template<template<typename...> typename Out_Tp, typename ... Out_Ts, template<typename...> typename Tup_Tp, template<typename...> typename Same_Tp, typename ... Arg_Ts>
		struct unique<Out_Tp<Out_Ts...>, Tup_Tp<>, Same_Tp, Arg_Ts...> { using type = Tup_Tp<Out_Ts...>; };
		
		template<template<typename...> typename Out_Tp, typename ... Out_Ts, template<typename...> typename Tup_Tp, typename T, typename ... Ts, template<typename...> typename Same_Tp, typename ... Arg_Ts>
		struct unique<Out_Tp<Out_Ts...>, Tup_Tp<T, Ts...>, Same_Tp, Arg_Ts...> : details::unique<std::conditional_t<std::disjunction_v<Same_Tp<T, Out_Ts, Arg_Ts...>...>, Out_Tp<Out_Ts...>, Out_Tp<Out_Ts..., T>>, Tup_Tp<Ts...>, Same_Tp, Arg_Ts...> { };
	}

	namespace details {
		template<template<typename...> typename Out_Tp, typename ... Out_Ts, template<typename...> typename Tup_Tp, template<typename...> typename Pred_Tp, typename ... Arg_Ts>
		struct filter<Out_Tp<Out_Ts...>, Tup_Tp<>, Pred_Tp, Arg_Ts...> { using type = Tup_Tp<Out_Ts...>; };

		template<template<typename...> typename Out_Tp, typename ... Out_Ts, template<typename...> typename Tup_Tp, typename T, typename ... Ts, template<typename...> typename Pred_Tp, typename ... Arg_Ts>
		struct filter<Out_Tp<Out_Ts...>, Tup_Tp<T, Ts...>, Pred_Tp, Arg_Ts...> : filter<std::conditional_t<Pred_Tp<T, Arg_Ts...>::value, Out_Tp<Out_Ts..., T>, Out_Tp<Out_Ts...>>, Tup_Tp<Ts...>, Pred_Tp, Arg_Ts...> { };
	}

	namespace details {
		template<template<typename...> typename Out_Tp, typename ... Out_Ts, template<typename...> typename Tup_Tp, template<typename...> typename Pred_Tp, typename ... Arg_Ts>
		struct remove_if<Out_Tp<Out_Ts...>, Tup_Tp<>, Pred_Tp, Arg_Ts...> { using type = Tup_Tp<Out_Ts...>; };

		template<template<typename...> typename Out_Tp, typename ... Out_Ts, template<typename...> typename Tup_Tp, typename T, typename ... Ts, template<typename...> typename Pred_Tp, typename ... Arg_Ts>
		struct remove_if<Out_Tp<Out_Ts...>, Tup_Tp<T, Ts...>, Pred_Tp, Arg_Ts...> : remove_if<std::conditional_t<Pred_Tp<T, Arg_Ts...>::value, Out_Tp<Out_Ts...>, Out_Tp<Out_Ts..., T>>, Tup_Tp<Ts...>, Pred_Tp, Arg_Ts...> { };
	}


	template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Pred_Tp, typename ... Arg_Ts>
	struct find_if<Tp<Ts...>, Pred_Tp, Arg_Ts...> {
	private:
		static constexpr bool valid[] = { Pred_Tp<Ts, Arg_Ts...>::value... };
	public:
		static constexpr size_t value = std::find(valid, valid + sizeof...(Ts), true) - valid;
		using type = arg_element_t<value, std::tuple<Ts...>>;;
	};
		
	namespace details {
		template<typename Tup, template<typename...> typename ... Eval_Tps>
		struct sort_eval;

		template<typename Tup>
		struct sort_eval<Tup> {
			static constexpr bool compare(std::size_t lhs, std::size_t rhs) { 
				return false;
			}
		};

		template<template<typename ...> typename Tp, typename T, typename ... Ts, template<typename...> typename Eval_Tp, template<typename...> typename ... Eval_Tps>
		struct sort_eval<Tp<T, Ts...>, Eval_Tp, Eval_Tps...> {
			static constexpr std::array<decltype(Eval_Tp<T>::value), 1 + sizeof...(Ts)> data{ Eval_Tp<T>::value, Eval_Tp<Ts>::value... };

			static constexpr bool compare(std::size_t lhs, std::size_t rhs) { 
				if (data[lhs] == data[rhs]) return sort_eval<Tp<T, Ts...>, Eval_Tps...>::compare(lhs, rhs);
				else return data[lhs] < data[rhs];
			}
		};

		

		template<template<typename...> typename Tp, typename ... Ts, std::size_t ... Is, template<typename...> typename ... Eval_Tps>
		struct sort_by<Tp<Ts...>, std::index_sequence<Is...>, Eval_Tps...>
		{
		private:
			static constexpr std::array<std::size_t, sizeof...(Ts)> indices = []{
				std::array<std::size_t, sizeof...(Ts)> arr = { Is... };
				std::ranges::sort(arr, sort_eval<Tp<Ts...>, Eval_Tps...>::compare);
				return arr;
			}();
		public:
			using type = Tp<arg_element_t<indices[Is], Tp<Ts...>>...>;
		};
	}

	namespace details {
		template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Eval_Tp, std::size_t ... Is, typename ... Arg_Ts>
		struct min_by<Tp<Ts...>, Eval_Tp, std::index_sequence<Is...>, Arg_Ts...>
		{
		private:
			using value_type = decltype(Eval_Tp<arg_element_t<0, Tp<Ts...>>, Arg_Ts...>::value);
			static constexpr std::array<value_type, sizeof...(Ts)> values = { Eval_Tp<Ts, Arg_Ts...>::value... };
		public:
			static constexpr std::size_t value = std::ranges::min_element(values) - values.begin();
			using type = arg_element_t<value, std::tuple<Ts...>>;
		};
	}

	namespace details {
		template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Eval_Tp, std::size_t ... Is, typename ... Arg_Ts>
		struct max_by<Tp<Ts...>, Eval_Tp, std::index_sequence<Is...>, Arg_Ts...>
		{
		private:
			using value_type = decltype(Eval_Tp<arg_element_t<0, Tp<Ts...>>, Arg_Ts...>::value);
			static constexpr std::array<value_type, sizeof...(Ts)> values = { Eval_Tp<Ts, Arg_Ts...>::value... };
		public:
			static constexpr std::size_t value = std::ranges::max_element(values) - values.begin();
			using type = arg_element_t<value, std::tuple<Ts...>>;
		};
	}

	namespace details { 
		template<typename Tup, typename Func_T, typename ... Arg_Ts, std::size_t ... Is> 
		inline constexpr decltype(auto) apply(Func_T&& func, std::index_sequence<Is...> ind, Arg_Ts&& ... args) {
			return func.template operator()<arg_element_t<Is, Tup>...>();
		}
	}

	template<typename Tup, typename Func_T, typename ... Arg_Ts> 
	inline constexpr decltype(auto) apply(Func_T&& func, Arg_Ts&& ... args) {
		return details::apply<Tup>(std::forward<Func_T>(func), std::make_index_sequence<arg_count_v<Tup>>{}, std::forward<Arg_Ts>(args)...);
	}
}

// arg_count
static_assert(ecs::meta::arg_count_v<std::tuple<int, char, float>> == 3);

// arg_element
static_assert(std::is_same_v<ecs::meta::arg_element_t<1, std::tuple<int, char, float>>, char>);

// eval
static_assert(std::is_same_v<ecs::meta::eval_t<int, std::add_const, std::add_lvalue_reference>, const int&>);

// eval_each
static_assert(std::is_same_v<ecs::meta::eval_each_t<std::tuple<int, char, float>, std::add_const>, std::tuple<const int, const char, const float>>);

// conjunction
static_assert(ecs::meta::conjunction_v<std::tuple<int, std::size_t, unsigned int>, std::is_integral>);

//disjunction
static_assert(ecs::meta::disjunction_v<std::tuple<int, char, float>, std::is_integral>);

// reverse
static_assert(std::is_same_v<ecs::meta::reverse_t<std::tuple<int, char, float>>, std::tuple<float, char, int>>);

// unique
static_assert(std::is_same_v<ecs::meta::unique_t<std::tuple<int, int, char, float>>, std::tuple<int, char, float>>);

// filter
static_assert(std::is_same_v<ecs::meta::filter_t<std::tuple<const int, const char, float>, std::is_const>, std::tuple<const int, const char>>);

// remove if
static_assert(std::is_same_v<ecs::meta::remove_if_t<std::tuple<const int, char, float>, std::is_const>, std::tuple<char, float>>);

// sort_by
static_assert(std::is_same_v<ecs::meta::sort_by_t<std::tuple<int, char, float>, ecs::meta::get_type_name>, std::tuple<char, float, int>>);

// min_by
static_assert(std::is_same_v<ecs::meta::min_by_t<std::tuple<int, char, float>, ecs::meta::get_type_name>, char>);

// max_by
static_assert(std::is_same_v<ecs::meta::max_by_t<std::tuple<int, char, float>, ecs::meta::get_type_name>, int>);

static_assert(std::is_same_v<ecs::meta::eval_try_t<int, std::is_same, void>, void>);

static_assert(ecs::meta::eval_try_t<int, std::is_same, void, int>::value);
