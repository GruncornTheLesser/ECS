#pragma once
#include <string_view>
#include <type_traits>
#include <array>
#include <algorithm>
#include <ranges>


#if defined(__clang__)
	#define PF_CMD __PRETTY_FUNCTION__
	#define PF_PREFIX "std::basic_string_view<char> ecs::meta::typeID() [T = "
	#define PF_SUFFIX "]"
#elif defined(__GNUC__) && !defined(__clang__)
	#define PF_CMD __PRETTY_FUNCTION__
	#define PF_PREFIX "constexpr std::basic_string_view<char> ecs::meta::typeID() [with T = "
	#define PF_SUFFIX "]"
#elif defined(_MSC_VER)
	#define PF_CMD __FUNCSIG__
	#define PF_PREFIX "struct std::basic_string_view<char> __cdecl ecs::meta::typeID<"
	#define PF_SUFFIX ">(void)"
#else
	#error "No support for this compiler."
#endif
namespace ecs::meta { // NOTE: when changing the namespace of this func you must update the macros for pretty function
	template<typename T> constexpr std::basic_string_view<char> typeID() {
		return { PF_CMD + sizeof(PF_PREFIX) - 1, sizeof(PF_CMD) + 1 - sizeof(PF_PREFIX) - sizeof(PF_SUFFIX) };
	}
	template<typename T> struct get_typeID { static constexpr std::basic_string_view<char> value = typeID<T>(); };

}
#undef PF_CMD
#undef PF_PREFIX
#undef PF_SUFFIX

namespace ecs::meta {
	template<typename Tup, template<typename...> typename Gt_Tp, typename ... Ts> struct each;
	template<typename Tup, template<typename...> typename Gt_Tp, typename ... Ts> using each_t = typename each<Tup, Gt_Tp, Ts...>::type;

	template<typename Tup, template<typename...> typename Gt_Tp, typename ... Ts> struct conjunction;
	template<typename Tup, template<typename...> typename Gt_Tp, typename ... Ts> static constexpr bool conjunction_v = conjunction<Tup, Gt_Tp, Ts...>::value;

	template<typename Tup, template<typename...> typename Gt_Tp, typename ... Ts> struct disjunction;
	template<typename Tup, template<typename...> typename Gt_Tp, typename ... Ts> static constexpr bool disjunction_v = disjunction<Tup, Gt_Tp, Ts...>::value;

	template<typename ... Tups> struct concat;
	template<typename ... Tups> using concat_t = typename concat<Tups...>::type;

	template<typename Tup, template<typename, typename> typename Cmp_Tp, typename ... Us> struct set_push;
	template<typename Tup, template<typename, typename> typename Cmp_Tp, typename ... Us> using set_push_t = typename set_push<Tup, Cmp_Tp, Us...>::type;

	template<typename Tup, template<typename> typename Cnd_Tp, typename ... Ts> struct push_if; 
	template<typename Tup, template<typename> typename Cnd_Tp, typename ... Ts> using push_if_t = typename push_if<Tup, Cnd_Tp, Ts...>::type;

	template<typename Tup, template<typename, typename> typename Same_Tp=std::is_same> struct unique;
	template<typename Tup, template<typename, typename> typename Same_Tp=std::is_same> using unique_t = typename unique<Tup, Same_Tp>::type;

	template<std::size_t I, typename ... Ts> struct element_index;
	template<std::size_t I, typename ... Ts> using element_index_t = typename element_index<I, Ts...>::type;

	template<typename Tup> struct element_count;
	template<typename Tup> static constexpr std::size_t element_count_v = element_count<Tup>::value;

	template<typename Tup, template<typename> typename Cnd_Tp> struct filter;
	template<typename Tup, template<typename> typename Cnd_Tp> using filter_t = typename filter<Tup, Cnd_Tp>::type;

	template<typename Tup, template<typename> typename Cnd_Tp> struct find_if;
	template<typename Tup, template<typename> typename Cnd_Tp> using find_if_t = typename find_if<Tup, Cnd_Tp>::type;
	template<typename Tup, template<typename> typename Cnd_Tp> static constexpr bool find_if_v = find_if<Tup, Cnd_Tp>::value;

	namespace details { template<typename Tup, template<typename> typename Gt_Tp, typename Ind> struct sort_by; };

	template<typename Tup, template<typename> typename Gt_Tp=get_typeID> using sort_by = details::sort_by<Tup, Gt_Tp, std::make_index_sequence<element_count<Tup>::value>>;
	template<typename Tup, template<typename> typename Gt_Tp=get_typeID> using sort_by_t = typename sort_by<Tup, Gt_Tp>::type;

	namespace details { template<typename Tup, template<typename> typename Gt_Tp, typename Ind> struct min_by; };

	template<typename Tup, template<typename> typename Gt_Tp=get_typeID> using min_by = details::min_by<Tup, Gt_Tp, std::make_index_sequence<element_count<Tup>::value>>;
	template<typename Tup, template<typename> typename Gt_Tp=get_typeID> using min_by_t = typename min_by<Tup, Gt_Tp>::type;

	namespace details { template<typename Tup, template<typename> typename Gt_Tp, typename Ind> struct max_by; };

	template<typename Tup, template<typename> typename Gt_Tp=get_typeID> using max_by = details::max_by<Tup, Gt_Tp, std::make_index_sequence<element_count<Tup>::value>>;
	template<typename Tup, template<typename> typename Gt_Tp=get_typeID> using max_by_t = typename max_by<Tup, Gt_Tp>::type;
}


namespace ecs::meta {
	template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Gt_Tp, typename ... Us> 
	struct each<Tp<Ts...>, Gt_Tp, Us...> { using type = Tp<typename Gt_Tp<Ts, Us...>::type...>; };

	template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Gt_Tp, typename ... Us> 
	struct conjunction<Tp<Ts...>, Gt_Tp, Us...> : std::conjunction<Gt_Tp<Ts, Us...>...> { };

	template<template<typename...> typename Tp, typename ... Ts, template<typename...> typename Gt_Tp, typename ... Us> 
	struct disjunction<Tp<Ts...>, Gt_Tp, Us...> : std::disjunction<Gt_Tp<Ts, Us...>...> { };

	template<template<typename ...> typename Tp, typename ... Ts, typename ... Us, typename ... Tups>
	struct concat<Tp<Ts...>, Tp<Us...>, Tups...> { using type = typename concat<Tp<Ts..., Us...>, Tups...>::type; };

	template<template<typename ...> typename Tp, typename ... Ts>
	struct concat<Tp<Ts...>> { using type = Tp<Ts...>; };



	template<template<typename...> typename Tp, typename ... Ts, template<typename, typename> typename Same_Tp, typename U, typename ... Us>
	struct set_push<Tp<Ts...>, Same_Tp, U, Us...> : set_push<std::conditional_t<std::disjunction_v<Same_Tp<U, Ts>...>, Tp<Ts...>, Tp<Ts..., U>>, Same_Tp, Us...> { };
	
	template<template<typename...> typename Tp, typename ... Ts, template<typename, typename> typename Same_Tp>
	struct set_push<Tp<Ts...>, Same_Tp> : std::type_identity<Tp<Ts...>> { };



	template<template<typename...> typename Tp, typename ... Ts, template<typename, typename> typename Same_Tp> 
	struct unique<Tp<Ts...>, Same_Tp> : set_push<Tp<>, Same_Tp, Ts...> { };



	template<template<typename...> typename Tp, typename ... Ts, template<typename> typename Cnd_Tp, typename U, typename ... Us>
	struct push_if<Tp<Ts...>, Cnd_Tp, U, Us...> : push_if<std::conditional_t<Cnd_Tp<U>::value, Tp<Ts..., U>, Tp<Ts...>>, Cnd_Tp, Us...> { };

	template<template<typename...> typename Tp, typename ... Ts, template<typename> typename Cnd_Tp>
	struct push_if<Tp<Ts...>, Cnd_Tp> : std::type_identity<Tp<Ts...>> { };

	template<template<typename...> typename Tp, typename ... Ts, template<typename> typename Cnd_Tp>
	struct filter<Tp<Ts...>, Cnd_Tp> : push_if<Tp<>, Cnd_Tp, Ts...> { };



	template<std::size_t I, typename T, typename ... Ts>
	struct element_index<I, T, Ts...> { using type = typename element_index<I - 1, Ts...>::type; };

	template<typename T, typename ... Ts>
	struct element_index<0, T, Ts...> { using type = T; };



	template<template<typename...> typename Tp, typename ... Ts>
	struct element_count<Tp<Ts...>> { static constexpr std::size_t value = sizeof...(Ts); };



	template<template<typename...> typename Tp, typename ... Ts, template<typename> typename Cnd_Tp>
	struct find_if<Tp<Ts...>, Cnd_Tp> {
	private:
		static constexpr bool valid[] = { Cnd_Tp<Ts>::value... };
	public:
		static constexpr size_t value = std::find(valid, valid + sizeof...(Ts), true) - valid;
		using type = typename element_index<value, Ts...>::type;
	};

	namespace details {
		template<template<typename...> typename Tp, typename T, typename ... Ts, template<typename> typename Gt_Tp, std::size_t ... Is>
		struct sort_by<Tp<T, Ts...>, Gt_Tp, std::index_sequence<Is...>>
		{
		//private:
			static constexpr std::array<decltype(Gt_Tp<T>::value), sizeof...(Is)> values = { 
				Gt_Tp<T>::value, Gt_Tp<Ts>::value... 
			};
			static constexpr std::array<std::size_t, sizeof...(Is)> indices = []{
				std::array<std::size_t, sizeof...(Is)> arr = { Is... };
				std::ranges::sort(arr, [](std::size_t lhs, std::size_t rhs) { 
					return values[lhs] < values[rhs];
				});
				return arr;
			}();
		public:
			using type = Tp<element_index_t<indices[Is], T, Ts...>...>;
		};
	}

	namespace details {
		template<template<typename...> typename Tp, typename ... Ts, template<typename> typename Gt_Tp, std::size_t ... Is>
		struct min_by<Tp<Ts...>, Gt_Tp, std::index_sequence<Is...>>
		{
		private:
			static constexpr auto values[] = { Gt_Tp<Ts>::value... };
		public:
			static constexpr std::size_t value = std::ranges::max(values) - values;
			using type = element_index_t<value, Ts...>;
		};
	}

	namespace details {
		template<template<typename...> typename Tp, typename ... Ts, template<typename> typename Gt_Tp, std::size_t ... Is>
		struct max_by<Tp<Ts...>, Gt_Tp, std::index_sequence<Is...>>
		{
		private:
			static constexpr auto values[] = { Gt_Tp<Ts>::value... };
		public:
			static constexpr std::size_t value = std::ranges::min(values) - values;
			using type = element_index_t<value, Ts...>;
		};
	}


	namespace details {
		template<typename Tup, typename Func_T, std::size_t ... Is>
		inline constexpr void apply(Func_T&& func, std::index_sequence<Is...>) {
			((func.template operator()<std::tuple_element_t<Is, Tup>>()), ...);
		}
	}
	template<typename Tup, typename Func_T>
	inline constexpr void apply(Func_T&& func) {
		details::apply<Tup>(std::forward<Func_T>(func), std::make_index_sequence<std::tuple_size_v<Tup>>{});
	}
}

